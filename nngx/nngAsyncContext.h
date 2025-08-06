#pragma once

#include "nngException.h"
#include "nngMsg.h"
#include "nngSocket.h"

namespace nng
{
	// AsyncContext 类：异步上下文基类，继承 Socket，提供异步消息处理支持
	// 用途：管理异步 I/O 操作，提供线程安全的消息发送机制
	// 特性：
	// - 使用 RAII 管理 Aio 对象
	// - 通过 std::recursive_mutex 确保线程安全
	// - 支持异步消息发送
	class AsyncContext : public Aio, virtual public Socket
	{
	protected:
		using _Ty_mutex = ::std::recursive_mutex;
		using _Ty_scoped_lock = ::std::scoped_lock<_Ty_mutex>;
		using _Ty_unique_lock = ::std::unique_lock<_Ty_mutex>;

	public:
		// 构造函数：初始化异步上下文
		// 参数：callback - 回调函数，callback_context - 回调上下文
		// 异常：若 Aio 分配失败，抛出 Exception
		AsyncContext(Aio::_Callback_t callback, void* callback_context) noexcept(false)
			: Aio(callback, callback_context) {
		}

		// 析构函数：释放异步上下文资源
		virtual ~AsyncContext() noexcept = default;

	protected:
		// 发送消息
		// 参数：msg - 要发送的 Msg 对象
		inline void _Send(Msg&& msg) noexcept {
			_My_aio_state = SEND;
			nng_aio_set_msg(*this, msg.release());
			send(*this);
		}

	protected:
		_Ty_mutex _My_mtx;
		enum { INIT, RECV, SEND, WAIT, IDLE } _My_aio_state = INIT;
	};

	// AsyncSender 类：异步发送基类，继承 AsyncContext，提供消息队列和回调处理
	// 用途：支持异步消息发送，管理消息队列并处理发送/接收回调
	// 特性：
	// - 使用消息队列（std::queue）管理待发送消息
	// - 提供虚函数接口以支持子类自定义发送/接收行为
	// - 线程安全，通过 AsyncContext 的互斥锁保护
	class AsyncSender : public AsyncContext
	{
	protected:
		typedef struct _MSG_ITEM
		{
			Msg _Msg;
			std::optional<std::promise<Msg>> _Promise_reply;
		} MSG_ITEM, * PMSG_ITEM;

	public:
		// 构造函数：初始化异步发送器
		// 异常：若 Aio 分配失败，抛出 Exception
		AsyncSender() noexcept(false) : AsyncContext(_Callback_sender, this) {}

		// 析构函数：释放异步发送器资源
		virtual ~AsyncSender() noexcept = default;

	protected:
		// 发送消息项
		// 参数：_Msg_item - 包含消息和可选回复承诺的消息项
		void _Send(MSG_ITEM&& _Msg_item) noexcept {
			_Ty_scoped_lock locker(_My_mtx);
			_My_msgs.push(std::move(_Msg_item));

			if (_My_aio_state == INIT || _My_aio_state == IDLE) {
				AsyncContext::_Send(std::move(_My_msgs.front()._Msg));
			}
		}

	private:
		// 回调函数：处理异步 I/O 操作的结果
		// 参数：callback_context - 回调上下文（指向 AsyncSender）
		static void _Callback_sender(void* callback_context) noexcept {
			auto _Sender = reinterpret_cast<AsyncSender*>(callback_context);
			_Ty_scoped_lock locker(_Sender->_My_mtx);
			assert(!_Sender->_My_msgs.empty());
			auto& _Msg_item_ref = _Sender->_My_msgs.front();

			nng_err e = _Sender->result();
			if (e == NNG_OK) {
				if (_Sender->_My_aio_state == SEND) {
					_Sender->_On_sender_sent(_Msg_item_ref);

					_Sender->release_msg();
					if (_Msg_item_ref._Promise_reply) {
						_Sender->_My_aio_state = RECV;
						_Sender->recv(*_Sender);
						return;
					}
				}
				else if (_Sender->_My_aio_state == RECV) {
					Msg _Msg_reply = _Sender->release_msg();
					_Sender->_On_sender_recv(_Msg_item_ref, _Msg_reply);
					_Msg_item_ref._Promise_reply->set_value(std::move(_Msg_reply));
				}

				_Sender->_My_msgs.pop();
				_Sender->_Send_next();
			}
			else {
				_Sender->_On_sender_exception(_Msg_item_ref, e);

				_Sender->release_msg();
				_Sender->_My_aio_state = IDLE;
			}
		}

		// 发送队列中的下一个消息
		void _Send_next() noexcept {
			_Ty_scoped_lock locker(_My_mtx);

			if (_My_msgs.empty()) {
				_My_aio_state = IDLE;
			}
			else {
				AsyncContext::_Send(std::move(_My_msgs.front()._Msg));
			}
		}

	private:
		// 虚函数：处理消息发送完成
		// 参数：_Msg_item - 发送的消息项
		virtual void _On_sender_sent(MSG_ITEM& _Msg_item) noexcept {}

		// 虚函数：处理发送异常
		// 参数：_Msg_item - 发送的消息项，e - 错误码
		virtual void _On_sender_exception(MSG_ITEM& _Msg_item, nng_err e) noexcept {}

		// 虚函数：处理接收到的回复消息
		// 参数：_Msg_item - 发送的消息项，m - 接收的回复消息
		virtual void _On_sender_recv(MSG_ITEM& _Msg_item, Msg& m) noexcept {}

	protected:
		std::queue<MSG_ITEM> _My_msgs;
	};

	// AsyncSenderNoReturn 类：无返回的异步发送器，继承 AsyncSender
	// 用途：提供无回复需求的异步消息发送功能
	// 特性：
	// - 支持多种消息格式（iov、Msg、带代码的 Msg）
	// - 继承 AsyncSender 的线程安全和队列管理
	class AsyncSenderNoReturn : public AsyncSender
	{
	public:
		// 异步发送 I/O 向量数据
		// 参数：iov - I/O 向量
		void async_send(const nng_iov& iov) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			_Send(std::move(mi));
		}

		// 异步发送消息
		// 参数：msg - 要发送的 Msg 对象
		void async_send(Msg&& msg) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			_Send(std::move(mi));
		}

		// 异步发送带消息代码的 I/O 向量数据
		// 参数：code - 消息代码，iov - I/O 向量
		void async_send(Msg::_Ty_msg_code code, const nng_iov& iov) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			Msg::_Append_msg_code(mi._Msg, code);
			_Send(std::move(mi));
		}

		// 异步发送带消息代码的消息
		// 参数：code - 消息代码，msg - 要发送的 Msg 对象
		void async_send(Msg::_Ty_msg_code code, Msg&& msg) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			Msg::_Append_msg_code(mi._Msg, code);
			_Send(std::move(mi));
		}
	};

	// AsyncSenderWithReturn 类：带返回的异步发送器，继承 AsyncSender
	// 用途：提供需要回复的异步消息发送功能，支持 std::future/promise
	// 特性：
	// - 支持异步消息发送并通过 std::future 获取回复
	// - 继承 AsyncSender 的线程安全和队列管理
	class AsyncSenderWithReturn : public AsyncSender
	{
	public:
		// 异步发送 I/O 向量数据并等待回复
		// 参数：iov - I/O 向量，promise - 用于存储回复的承诺
		void async_send(const nng_iov& iov, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// 异步发送消息并等待回复
		// 参数：msg - 要发送的 Msg 对象，promise - 用于存储回复的承诺
		void async_send(Msg&& msg, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// 异步发送带消息代码的 I/O 向量数据并等待回复
		// 参数：code - 消息代码，iov - I/O 向量，promise - 用于存储回复的承诺
		void async_send(Msg::_Ty_msg_code code, const nng_iov& iov, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			Msg::_Append_msg_code(mi._Msg, code);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// 异步发送带消息代码的消息并等待回复
		// 参数：code - 消息代码，msg - 要发送的 Msg 对象，promise - 用于存储回复的承诺
		void async_send(Msg::_Ty_msg_code code, Msg&& msg, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			Msg::_Append_msg_code(mi._Msg, code);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// 异步发送消息并返回 future
		// 参数：msg - 要发送的 Msg 对象
		// 返回：std::future 用于获取回复消息
		std::future<Msg> async_send(Msg&& msg) noexcept(false) {
			std::promise<Msg> promise;
			auto future = promise.get_future();
			async_send(std::move(msg), std::move(promise));
			return future;
		}

		// 异步发送带消息代码的消息并返回 future
		// 参数：code - 消息代码，msg - 要发送的 Msg 对象
		// 返回：std::future 用于获取回复消息
		std::future<Msg> async_send(Msg::_Ty_msg_code code, Msg&& msg) noexcept(false) {
			std::promise<Msg> promise;
			auto future = promise.get_future();
			async_send(code, std::move(msg), std::move(promise));
			return future;
		}
	};
}