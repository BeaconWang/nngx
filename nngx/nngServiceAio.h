#pragma once

#include "nngException.h"
#include "nngDispatcher.h"
#include "nngAio.h"
#include "nngx.h"

namespace nng
{
	// ServiceAio 类：基于 NNG Aio 的服务模板类，继承自指定的基类
	// 用途：提供对 NNG 服务的异步调度管理，使用 nng_aio 而非线程实现异步操作
	// 特性：
	// - 基于模板参数 _TyBase 扩展功能，适用于不同类型的 NNG 连接器（如 Listener 或 Dialer）
	// - 使用 RAII 管理 aio 资源，确保在析构时自动释放
	// - 支持异步调度，通过 nng_aio 回调机制实现异步消息处理
	// - 提供启动和停止异步调度的功能
	template <typename _TyBase>
	class ServiceAio
		: public _TyBase
	{
	public:
		// 析构函数：停止异步调度并清理资源
		virtual ~ServiceAio()
		{
			stop_dispatch();
		}

	public:
		// 启动异步调度
		// 参数：addr - 服务监听或连接的地址，flags - 启动标志，默认为 0，cb - 发起连接之前的回调
		// 返回：操作结果，0 表示成功，非 0 表示失败
		// 异常：若基类启动失败，可能抛出 Exception
		template <typename _Peer_t>
		int start_dispatch(
			std::string_view addr,
			int flags = 0,
			std::function<void(_Peer_t&)> cb = {}) noexcept(false)
		{
			int rv = _TyBase::start(addr, flags, cb);
			if (rv != NNG_OK) {
				return rv;
			}

			// 创建 aio 对象用于异步调度
			_My_aio = std::make_unique<Aio>(_Aio_callback, this);
			if (!_My_aio) {
				return NNG_ENOMEM;
			}

			// 开始异步调度
			_Start_async_dispatch();

			return NNG_OK;
		}

		int start_dispatch(
			std::string_view addr,
			int flags = 0) noexcept(false)
		{
			int rv = _TyBase::start(addr, flags);
			if (rv != NNG_OK) {
				return rv;
			}

			// 创建 aio 对象用于异步调度
			_My_aio = std::make_unique<Aio>(_Aio_callback, this);
			if (!_My_aio) {
				return NNG_ENOMEM;
			}

			// 开始异步调度
			_Start_async_dispatch();

			return NNG_OK;
		}

		// 检查异步调度是否正在运行
		// 返回：true 表示正在运行，false 表示已停止
		inline bool is_running() const noexcept
		{
			return _My_running.load();
		}

		// 停止异步调度
		// 返回：true 表示成功停止，false 表示已经停止
		// 说明：调用基类的 close 方法关闭连接器，并停止异步调度
		bool stop_dispatch() noexcept
		{
			if (!_My_running.load()) {
				return false;
			}

			// 停止异步调度
			_My_running.store(false);

			// 取消当前的 aio 操作
			if (_My_aio) {
				_My_aio->cancel();
			}

			// 关闭基类连接器
			_TyBase::close();

			// 清理 aio 对象
			_My_aio.reset();

			return true;
		}

		// 等待异步调度完成
		// 说明：等待当前正在进行的异步操作完成
		void service_wait() noexcept
		{
			if (_My_aio) {
				_My_aio->wait();
			}
		}

        // 获取当前的 aio 对象
		const std::unique_ptr<Aio>& service_aio() const noexcept
		{
			return _My_aio;
        }
	protected:
		// 开始异步调度
		// 说明：启动异步消息接收循环
		void _Start_async_dispatch() noexcept
		{
			_My_running.store(true);
			_Receive_next();
		}

		// 接收下一条消息
		// 说明：异步接收消息并设置回调
		void _Receive_next() noexcept
		{
			if (!_My_running.load() || !_My_aio) {
				return;
			}

			try {
				// 使用 aio 异步接收消息
				_TyBase::recv(*_My_aio);
			}
			catch (const Exception& e) {
				// 处理接收异常
				if (this->_On_dispatch_exception(e)) {
					_My_running.store(false);
				}
				else {
					// 继续尝试接收
					_Receive_next();
				}
			}
		}
	private:
		// Aio 回调函数
		// 参数：arg - 回调上下文（ServiceAio 对象指针）
		// 说明：处理异步接收完成的事件
		static void _Aio_callback(void* arg) noexcept
		{
			auto* pService = static_cast<ServiceAio*>(arg);
			if (!pService) {
                assert(false);
				return;
			}

			pService->_Do_callback();
		}
        // Aio 回调处理函数
        // 说明：检查 aio 结果，处理消息或错误，并继续接收
		void _Do_callback() noexcept
		{
			// 检查 aio 操作结果
			if (_My_aio->result() != NNG_OK) {
				// 处理接收错误
				if (this->_On_dispatch_exception({ _My_aio->result(), "recv_aio" })) {
					_My_running.store(false);
					return;
				}
			}
			else {
				// 获取接收到的消息
				Msg m = _My_aio->get_msg();
				if (m) {
					// 处理消息
					if (!this->_On_raw_message(m)) {
						auto code = Msg::_Chop_msg_code(m);
						auto result = this->_On_message(code, m);
						Msg::_Append_msg_result(m, result);
					}

					if (_My_running.load()) {
						if constexpr (std::is_base_of_v<DispatcherWithReturn, _TyBase>) {
                            // 如果需要回复，则发送回复消息
							_My_aio->set_msg(std::move(m));
							_TyBase::send(*_My_aio);
						}
						else {
                            // 否则继续接收下一条消息
							_Receive_next();
						}
					}
				}
				else {
                    // 消息为空，继续接收下一条
					if (_My_running.load()) {
						_Receive_next();
					}
				}
			}
        }
	private:
		std::unique_ptr<Aio> _My_aio;           // 异步 I/O 对象
		std::atomic<bool> _My_running{false};   // 运行状态标志
	};
} 