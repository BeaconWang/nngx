#pragma once

#include <list>
#include <mutex>
#include <queue>
#include <future>
#include <optional>

#include "nngPeer.h"
#include "nngListener.h"
#include "nngDialer.h"
#include "nngMsg.h"
#include "nngAio.h"
#include "nngCtx.h"
#include "nngService.h"

/*
__________
\______   \ ____ _____    ____  ____   ____
 |    |  _// __ \\__  \ _/ ___\/  _ \ /    \
 |    |   \  ___/ / __ \\  \__(  <_> )   |  \
 |______  /\___  >____  /\___  >____/|___|  /
		\/     \/     \/     \/           \/
		-- Create.Beacon.20250521
		-- Modify.Beacon.20250623
			-> 1. Support for advanced restricted access (access security descriptor)
		-- Modify.Beacon.20250627
			-> 1. Comaptible suport nng 1.11
		-- Modify.Beacon.20250701
			-> 1. Fix comment
			-> 2. Add Msg::body_tail function to fetch data from the tail at a specified offset
		-- Modify.Beacon.20250702
			-> 1. Add SocketCore class to manage nng_socket
			-> 2. Add SocketOpt class to manage nng_socket options
			-> 3. Add Socket class to manage nng_socket and SocketOpt
*/

/*

		virtual void _Prepare_connect() {}
		virtual void _On_sender_sent(MSG_ITEM& _Msg_item) noexcept;
		virtual void _On_sender_exception(MSG_ITEM& _Msg_item, nng_err e) noexcept;
		virtual void _On_sender_recv(MSG_ITEM& _Msg_item, Msg& m) noexcept;

		virtual void _On_close(PWORK_ITEM _Work_item);
		virtual bool _On_raw_message(Msg& msg);
		virtual bool _On_raw_message(Msg& msg, nng_duration& _Wait_ms);
		virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg);
		virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg, nng_duration& _Wait_ms);
		virtual void _On_wait(nng_duration& _Wait_ms);
*/

namespace nng
{
	// 初始化 NNG 库
	// 返回：操作结果，0 表示成功
	inline int initialize() noexcept {
		return NNG_OK;
	}

	// 释放 NNG 库资源
	inline void uninitialize() noexcept {
		nng_fini();
	}
}

namespace nng
{
	// AsyncContext 类：异步上下文基类，继承 Socket，提供异步消息处理支持
	// 用途：管理异步 I/O 操作，提供线程安全的消息发送机制
	// 特性：
	// - 使用 RAII 管理 Aio 对象
	// - 通过 std::recursive_mutex 确保线程安全
	// - 支持异步消息发送
	class AsyncContext : virtual public Socket
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
			: _My_aio(callback, callback_context) {
		}

		// 析构函数：释放异步上下文资源
		virtual ~AsyncContext() noexcept = default;

	protected:
		// 发送消息
		// 参数：msg - 要发送的 Msg 对象
		inline void _Send(Msg&& msg) noexcept {
			_My_aio_state = SEND;
			nng_aio_set_msg(_My_aio, msg.release());
			send(_My_aio);
		}

	protected:
		Aio _My_aio = nullptr;
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

			nng_err e = _Sender->_My_aio.result();
			if (e == NNG_OK) {
				if (_Sender->_My_aio_state == SEND) {
					_Sender->_On_sender_sent(_Msg_item_ref);

					_Sender->_My_aio.release_msg();
					if (_Msg_item_ref._Promise_reply) {
						_Sender->_My_aio_state = RECV;
						_Sender->recv(_Sender->_My_aio);
						return;
					}
				}
				else if (_Sender->_My_aio_state == RECV) {
					Msg _Msg_reply = _Sender->_My_aio.release_msg();
					_Sender->_On_sender_recv(_Msg_item_ref, _Msg_reply);
					_Msg_item_ref._Promise_reply->set_value(std::move(_Msg_reply));
				}

				_Sender->_My_msgs.pop();
				_Sender->_Send_next();
			}
			else {
				_Sender->_On_sender_exception(_Msg_item_ref, e);

				_Sender->_My_aio.release_msg();
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

namespace nng
{
    // Dispatcher 类：消息分发基类，提供消息处理虚函数接口
    // 用途：定义消息处理回调，供派生类实现具体分发逻辑
    // 特性：
    // - 提供处理原始消息和编码消息的虚函数
    // - 支持异常处理回调
    class Dispatcher
    {
    protected:
        // 虚函数：处理原始消息
        // 参数：msg - 接收的 Msg 对象
        // 返回：true 表示消息已处理，false 表示继续处理
        virtual bool _On_raw_message(Msg& msg) { return false; }

        // 虚函数：处理编码消息
        // 参数：code - 消息代码，msg - 接收的 Msg 对象
        // 返回：消息处理结果
        virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) { return {}; }

        // 虚函数：处理分发异常
        // 参数：e - 异常对象
        // 返回：true 表示停止分发，false 表示继续
        virtual bool _On_dispatch_exception(const Exception& e) { return true; }
    };

	// DispatcherNoReturn 类：无返回的消息分发器，继承 Dispatcher 和 Socket
	// 用途：处理接收的消息，无需发送回复
	// 特性：
	// - 循环接收并处理消息
	// - 支持异常处理
	class DispatcherNoReturn : public Dispatcher, virtual public Socket
	{
	public:
		// 分发消息
		// 循环接收消息并调用处理回调，直至异常终止
		void dispatch() noexcept {
			for (;;) {
				try {
					Msg m = recv();
					_On_receiver_recv(m);
				}
				catch (const Exception& e) {
					if (_On_dispatch_exception(e)) {
						break;
					}
				}
			}
		}

	private:
		// 虚函数：处理接收到的消息
		// 参数：m - 接收的 Msg 对象
		// 返回：true 表示消息已处理，false 表示继续处理
		virtual bool _On_receiver_recv(Msg& m) noexcept {
			if (!_On_raw_message(m)) {
				auto code = Msg::_Chop_msg_code(m);
				_On_message(code, m);
			}
			return false;
		}
	};

	// DispatcherWithReturn 类：带返回的消息分发器，继承 Dispatcher 和 Socket
	// 用途：处理接收的消息并发送回复
	// 特性：
	// - 循环接收、处理消息并发送回复
	// - 支持异常处理
	class DispatcherWithReturn : public Dispatcher, virtual public Socket
	{
	public:
		// 分发消息
		// 循环接收消息、处理并发送回复，直至异常终止
		void dispatch() noexcept {
			for (;;) {
				try {
					Msg m = recv();
					_On_receiver_recv(m);
					send(std::move(m));
				}
				catch (const Exception& e) {
					if (_On_dispatch_exception(e)) {
						break;
					}
				}
			}
		}

	private:
		// 虚函数：处理接收到的消息
		// 参数：m - 接收的 Msg 对象
		// 返回：true 表示消息已处理，false 表示继续处理
		virtual bool _On_receiver_recv(Msg& m) noexcept {
			if (!_On_raw_message(m)) {
				auto code = Msg::_Chop_msg_code(m);
				auto result = _On_message(code, m);
				Msg::_Append_msg_result(m, result);
			}
			return false;
		}
	};
}

namespace nng
{
	// Pull 类：Pull 协议的模板类，继承 Peer、Socket 和 DispatcherNoReturn
	// 用途：实现 Pull 协议的异步消息接收
	// 特性：
	// - 支持 Listener 或 Dialer 连接器
	// - 提供无返回的消息分发
	template <class _Connector_t = Listener>
	class Pull : public Peer<_Connector_t>, virtual public Socket, public DispatcherNoReturn
	{
		// 创建 Pull 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_pull0_open);
		}
	};

	// Push 类：Push 协议的模板类，继承 Peer、Socket 和 AsyncSenderNoReturn
	// 用途：实现 Push 协议的异步消息发送
	// 特性：
	// - 支持 Listener 或 Dialer 连接器
	// - 提供无返回的异步发送
	template <class _Connector_t = Dialer>
	class Push : public Peer<_Connector_t>, virtual public Socket, public AsyncSenderNoReturn
	{
		// 创建 Push 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_push0_open);
		}
	};
}

namespace nng
{
	// Pair 类：Pair 协议的模板类，继承 Peer、Socket、DispatcherNoReturn 和 AsyncSenderNoReturn
	// 用途：实现 Pair 协议的异步消息发送和接收
	// 特性：
	// - 支持 Listener 或 Dialer 连接器
	// - 提供无返回的发送和分发
	template <class _Connector_t>
	class Pair : public Peer<_Connector_t>, virtual public Socket, public DispatcherNoReturn, public AsyncSenderNoReturn
	{
		// 创建 Pair 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_pair0_open);
		}
	};
}

namespace nng
{
	// Response 类：Rep 协议类，继承 Peer 和 DispatcherWithReturn
	// 用途：实现 Rep 协议的消息接收和回复
	// 特性：
	// - 使用 Listener 连接器
	// - 提供带返回的消息分发
	class Response : public Peer<Listener>, virtual public Socket, public DispatcherWithReturn
	{
		// 创建 Rep 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_rep0_open);
		}
	};

	// Request 类：Req 协议类，继承 Peer 和 AsyncSenderWithReturn
	// 用途：实现 Req 协议的异步消息发送和回复接收
	// 特性：
	// - 使用 Dialer 连接器
	// - 提供带返回的异步发送
	class Request : public Peer<Dialer>, virtual public Socket, public AsyncSenderWithReturn
	{
		// 创建 Req 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_req0_open);
		}

	public:
		// 简单发送消息并接收回复
		// 参数：addr - 连接地址，msg - 消息对象，pre_send - 发送前回调
		// 返回：操作结果，0 表示成功
		static int simple_send(std::string_view addr, Msg& msg, std::function<void(Request&)> pre_send = {}) noexcept(false) {
			Request req;
			int rv = req.start(addr);
			if (rv != NNG_OK) {
				return rv;
			}

			if (pre_send) {
				pre_send(req);
			}

			return req.send(msg);
		}

		// 简单发送带消息代码的消息并接收结果
		// 参数：addr - 连接地址，code - 消息代码，msg - 消息对象，pre_send - 发送前回调
		// 返回：消息处理结果
		// 异常：若创建或发送失败，抛出 Exception
		static Msg::_Ty_msg_result simple_send(std::string_view addr, Msg::_Ty_msg_code code, Msg& msg, std::function<void(Request&)> pre_send = {}) noexcept(false) {
			Request req;
			int rv = req.start(addr);
			if (rv != NNG_OK) {
				throw Exception(rv, "realloc");
			}

			if (pre_send) {
				pre_send(req);
			}

			return req.send(code, msg);
		}
	};

}

namespace nng
{
	// Publisher 类：Pub 协议的模板类，继承 Peer 和 AsyncSenderNoReturn
	// 用途：实现 Pub 协议的异步消息发布
	// 特性：
	// - 支持 Listener 或 Dialer 连接器
	// - 提供无返回的异步发送
	template <class _Connector_t = Listener>
	class Publisher : public Peer<_Connector_t>, virtual public Socket, public AsyncSenderNoReturn
	{
		// 创建 Pub 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_pub0_open);
		}
	};

	// Subscriber 类：Sub 协议的模板类，继承 Peer 和 DispatcherNoReturn
	// 用途：实现 Sub 协议的异步消息订阅
	// 特性：
	// - 支持 Listener 或 Dialer 连接器
	// - 提供无返回的消息分发
	template <class _Connector_t = Dialer>
	class Subscriber : public Peer<_Connector_t>, virtual public Socket, public DispatcherNoReturn
	{
		// 创建 Sub 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_sub0_open);
		}
	};

}

namespace nng
{
	// Survey 类：Surveyor 协议的模板类，继承 Peer 和 AsyncSenderWithReturn
	// 用途：实现 Surveyor 协议的异步消息发送和多回复接收
	// 特性：
	// - 支持 Listener 或 Dialer 连接器
	// - 提供带返回的异步发送
	template <class _Connector_t = Listener>
	class Survey : public Peer<_Connector_t>, virtual public Socket, public AsyncSenderWithReturn
	{
		// 创建 Surveyor 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_surveyor0_open);
		}

	public:
		// 发送消息并接收多个回复
		// 参数：msg - 要发送的 Msg 对象，iter - 迭代器用于存储回复
		// 返回：操作结果，0 表示成功
		// 异常：若接收超时以外的错误，抛出 Exception
		template <typename _Iter_t>
		int send(Msg&& msg, _Iter_t iter) noexcept(false) {
			int rv = Socket::send(std::move(msg));
			if (rv != NNG_OK) {
				return rv;
			}

			for (;;) {
				try {
					*iter = Socket::recv();
					++iter;
				}
				catch (const Exception& e) {
					if (e.get_error() != NNG_ETIMEDOUT) {
						rv = e.get_error();
					}
					break;
				}
			}

			return rv;
		}

		// 发送带消息代码的消息并接收多个回复
		// 参数：code - 消息代码，msg - 要发送的 Msg 对象，iter - 迭代器用于存储回复
		// 返回：操作结果，0 表示成功
		// 异常：若接收超时以外的错误，抛出 Exception
		template <typename _Iter_t>
		int send(Msg::_Ty_msg_code code, Msg&& msg, _Iter_t iter) noexcept(false) {
			Msg::_Append_msg_code(msg, code);
			return send(std::move(msg), iter);
		}
	};

	// Respond 类：Respondent 协议的模板类，继承 Peer 和 DispatcherWithReturn
	// 用途：实现 Respondent 协议的消息接收和回复
	// 特性：
	// - 支持 Listener 或 Dialer 连接器
	// - 提供带返回的消息分发
	template <class _Connector_t = Dialer>
	class Respond : public Peer<_Connector_t>, virtual public Socket, public DispatcherWithReturn
	{
		// 创建 Respondent 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_respondent0_open);
		}
	};
}

namespace nng
{
	// Bus 类：Bus 协议类，继承 Peer、Socket、DispatcherNoReturn 和 AsyncSenderNoReturn
	// 用途：实现 Bus 协议的异步消息发送和接收
	// 特性：
	// - 使用 Listener 连接器，支持动态添加 Dialer
	// - 提供无返回的发送和分发
	class Bus : public Peer<Listener>, virtual public Socket, public DispatcherNoReturn, public AsyncSenderNoReturn
	{
		// 创建 Bus 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_bus0_open);
		}

	public:
		// 添加拨号器并连接
		// 参数：addr - 连接地址
		// 返回：操作结果，0 表示成功
		// 异常：若拨号器创建失败，抛出 Exception
		int dial(std::string_view addr) noexcept(false) {

			Dialer dialer(*this, addr);
			int rv = dialer.start();
			if (rv != NNG_OK) {
				return rv;
			}

			_My_dialers.push_back(std::move(dialer));
			return rv;
		}

	private:
		std::list<Dialer> _My_dialers;
	};

}

namespace nng
{
	// ResponseParallel 类：并行 Rep 协议类，继承 Peer 和 DispatcherWithReturn
	// 用途：实现 Rep 协议的并行消息处理，支持多个工作项
	// 特性：
	// - 使用 Listener 连接器
	// - 通过多个工作项（WORK_ITEM）并行处理消息
	// - 提供带返回的消息分发
	class ResponseParallel : public Peer<Listener>, virtual public Socket, public DispatcherWithReturn
	{
		// 创建 Rep 协议套接字
		// 返回：操作结果，0 表示成功
		virtual int _Create() noexcept override {
			return Socket::create(nng_rep0_open);
		}

	protected:
		typedef struct _WORK_ITEM
		{
			enum
			{
				WIS_INIT,
				WIS_RECV,
				WIS_SEND,
				WIS_WAIT
			} _State = WIS_INIT;
			Aio _Aio;
			Ctx _Ctx;
			Msg _Msg;
			void* _Owner;

			// 工作项构造函数
			// 参数：socket - 套接字，callback - 回调函数，owner - 父对象
			// 异常：若 Aio 或 Ctx 创建失败，抛出 Exception
			explicit _WORK_ITEM(nng_socket socket, void (*callback)(void*), void* owner) noexcept(false)
				: _Aio(callback, this), _Ctx(socket), _Owner(owner) {
			}

			// 异步发送消息
			// 参数：msg - 要发送的 Msg 对象
			inline void send() noexcept {
				send(std::move(_Msg));
			}

			// 异步发送消息
			// 参数：msg - 要发送的 Msg 对象
			inline void send(Msg&& msg) noexcept {
				_Aio.set_msg(std::move(msg));
				_State = WIS_SEND;
				_Ctx.send(_Aio);
			}

			// 异步等待
			// 参数：ms - 等待时间（毫秒）
			inline void wait(nng_duration ms) noexcept {
				_State = WIS_WAIT;
				_Aio.sleep(ms);
			}
		} WORK_ITEM, * PWORK_ITEM;

	public:
		// 启动并行处理
		// 参数：addr - 监听地址，parallel - 并行工作项数量，flags - 启动标志
		// 返回：操作结果，0 表示成功
		// 异常：若创建或启动失败，抛出 Exception
		int start(std::string_view addr, size_t parallel = 1, int flags = 0) noexcept(false) {
			int rv = _Create();
			if (rv != NNG_OK) {
				return rv;
			}

			Peer<Listener>::_My_connector = std::make_unique<Listener>(*this, addr);
			rv = Peer<Listener>::_My_connector->start(flags);
			if (rv != NNG_OK) {
				return rv;
			}

			_My_work_items.reserve(parallel);

			for (size_t i = 0; i < parallel; ++i) {
				_My_work_items.emplace_back(*this, _Callback, this);
			}

			for (auto& _Work_item_ref : _My_work_items) {
				_Callback(&_Work_item_ref);
			}

			return NNG_OK;
		}

	protected:
		// 回调函数：处理工作项的异步操作
		// 参数：arg - 工作项指针
		static void _Callback(void* arg) noexcept {
			nng_err err;
			auto _Work_item = static_cast<PWORK_ITEM>(arg);
			auto* _This = static_cast<ResponseParallel*>(_Work_item->_Owner);

			switch (_Work_item->_State) {
			case WORK_ITEM::WIS_RECV:
				err = _Work_item->_Aio.result();
				switch (err) {
				case NNG_OK:
					_Work_item->_Msg = _Work_item->_Aio.release_msg();
					_This->_On_message(_Work_item);
					break;
				case NNG_ECLOSED:
					_This->_On_close(_Work_item);
					break;
				default:
					_This->_On_exception(err);
					break;
				}
				break;
			case WORK_ITEM::WIS_WAIT:
				_This->_On_wait(_Work_item);
				break;
			case WORK_ITEM::WIS_SEND:
				err = _Work_item->_Aio.result();
				switch (err) {
				case NNG_OK:
					break;
				case NNG_ECLOSED:
					_This->_On_close(_Work_item);
					break;
				default:
					_This->_On_exception(err);
					break;
				}
			case WORK_ITEM::WIS_INIT:
				_Work_item->_State = WORK_ITEM::WIS_RECV;
				_Work_item->_Ctx.recv(_Work_item->_Aio);
				break;
			default:
				_This->_On_exception(NNG_ESTATE);
				break;
			}
		}

	private:
		// 虚函数：处理工作项关闭
		// 参数：_Work_item - 工作项指针
		virtual void _On_close(PWORK_ITEM _Work_item) {}

		// 虚函数：处理原始消息
		// 参数：msg - 接收的 Msg 对象
		// 返回：true 表示消息已处理，false 表示继续处理
		virtual bool _On_raw_message(Msg& msg) override { return false; }

		// 虚函数：处理原始消息并设置等待时间
		// 参数：msg - 接收的 Msg 对象，_Wait_ms - 等待时间
		// 返回：true 表示消息已处理，false 表示继续处理
		virtual bool _On_raw_message(Msg& msg, nng_duration& _Wait_ms) {
			return _On_raw_message(msg);
		}

		// 虚函数：处理编码消息
		// 参数：code - 消息代码，msg - 接收的 Msg 对象
		// 返回：消息处理结果
		virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
			return {};
		}

		// 虚函数：处理编码消息并设置等待时间
		// 参数：code - 消息代码，msg - 接收的 Msg 对象，_Wait_ms - 等待时间
		// 返回：消息处理结果
		virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg, nng_duration& _Wait_ms) {
			return _On_message(code, msg);
		}

		// 虚函数：处理等待状态
		// 参数：_Wait_ms - 等待时间
		virtual void _On_wait(nng_duration& _Wait_ms) {}

		// 处理等待状态的工作项
		// 参数：_Work_item - 工作项指针
		virtual void _On_wait(PWORK_ITEM _Work_item) {
			nng_duration _Wait_ms = -1;
			_On_wait(_Wait_ms);

			if (_Wait_ms < 0) {
				_Work_item->send();
			}
			else {
				_Work_item->wait(_Wait_ms);
			}
		}

		// 处理接收到的消息
		// 参数：_Work_item - 工作项指针
		void _On_message(PWORK_ITEM _Work_item) {
			Msg& msg = _Work_item->_Msg;
			nng_duration _Wait_ms = -1;
			if (!_On_raw_message(msg, _Wait_ms)) {
				Msg::_Ty_msg_code code = Msg::_Chop_msg_code(msg);
				auto res = _On_message(code, msg, _Wait_ms);
				Msg::_Append_msg_result(msg, res);
			}

			if (_Wait_ms < 0) {
				_Work_item->send();
			}
			else {
				_Work_item->wait(_Wait_ms);
			}
		}

		// 虚函数：处理异常
		// 参数：e - 错误码
		virtual void _On_exception(nng_err e) {}

	protected:
		std::vector<WORK_ITEM> _My_work_items;
	};
}