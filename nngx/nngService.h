#pragma once

#include "nngException.h"

namespace nng
{
	// Service 类：NNG 服务模板类，继承自指定的基类
	// 用途：提供对 NNG 服务的异步调度管理，支持启动和停止调度线程
	// 特性：
	// - 基于模板参数 _TyBase 扩展功能，适用于不同类型的 NNG 连接器（如 Listener 或 Dialer）
	// - 使用 RAII 管理调度线程，确保在析构时自动停止
	// - 支持异步调度，通过独立线程调用基类的 dispatch 方法
	template <typename _TyBase>
	class Service
		: public _TyBase
	{
	public:
		// 析构函数：停止调度线程并清理资源
		virtual ~Service()
		{
			stop_dispatch();
		}
	public:
		// 启动调度线程
		// 参数：addr - 服务监听或连接的地址，flags - 启动标志，默认为 0
		// 返回：操作结果，0 表示成功，非 0 表示失败
		// 异常：若基类启动失败，可能抛出 Exception
		int start_dispatch(std::string_view addr, int flags = 0) noexcept(false)
		{
			int rv = _TyBase::start(addr, flags);
			if (rv != NNG_OK) {
				return rv;
			}

			_My_dispatch_thread = std::thread(
				[=]
				{
					_TyBase::dispatch();
				}
			);

			return NNG_OK;
		}

		// 检查调度线程是否可加入
		// 返回：true 表示线程可加入，false 表示不可加入
		inline bool joinable() noexcept
		{
			return _My_dispatch_thread.joinable();
		}

		// 加入调度线程，等待其完成。
		// 注意：仅在线程可加入时调用。
		inline void join() {
			_My_dispatch_thread.join();
		}

		// 停止调度线程
		// 返回：true 表示成功停止并加入线程，false 表示线程不可加入
		// 说明：调用基类的 close 方法关闭连接器，并等待调度线程结束
		bool stop_dispatch() noexcept
		{
			if (!_My_dispatch_thread.joinable()) {
				return false;
			}

			_TyBase::close();

			_My_dispatch_thread.join();

			return true;
		}
	protected:
		std::thread _My_dispatch_thread;    // _Tcp_base_parser -> virtual class
	};
}