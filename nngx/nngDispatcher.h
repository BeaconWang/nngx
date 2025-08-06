#pragma once

#include "nngException.h"
#include "nngMsg.h"
#include "nngSocket.h"

namespace nng
{
	// Dispatcher 类：消息分发基类，提供消息处理虚函数接口
	// 用途：定义消息处理回调，供派生类实现具体分发逻辑
	// 特性：
	// - 提供处理原始消息和编码消息的虚函数
	// - 支持异常处理回调
	class Dispatcher : virtual public Socket
	{
	public:
		// 分发消息
		// 循环接收消息并调用处理回调，直至异常终止
		void dispatch() noexcept {
			for (;;) {
				try {
					Msg m = recv();
					_On_recv(m);
				}
				catch (const Exception& e) {
					if (_On_dispatch_exception(e)) {
						break;
					}
				}
			}
		}
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

	private:
		// 虚函数：处理接收到的消息
		// 参数：m - 接收的 Msg 对象
		virtual void _On_recv(Msg& m) noexcept = 0;
	};

	// DispatcherNoReturn 类：无返回的消息分发器，继承 Dispatcher 和 Socket
	// 用途：处理接收的消息，无需发送回复
	// 特性：
	// - 循环接收并处理消息
	// - 支持异常处理
	class DispatcherNoReturn : public Dispatcher
	{

	protected:
		virtual void _On_recv(Msg& m) noexcept override {
			if (!_On_raw_message(m)) {
				auto code = Msg::_Chop_msg_code(m);
				_On_message(code, m);
			}
		}
	};

	// DispatcherWithReturn 类：带返回的消息分发器，继承 Dispatcher 和 Socket
	// 用途：处理接收的消息并发送回复
	// 特性：
	// - 循环接收、处理消息并发送回复
	// - 支持异常处理
	class DispatcherWithReturn : public Dispatcher
	{
	protected:
		virtual void _On_recv(Msg& m) noexcept override {
			if (!_On_raw_message(m)) {
				auto code = Msg::_Chop_msg_code(m);
				auto result = _On_message(code, m);
				Msg::_Append_msg_result(m, result);
			}

			send(std::move(m));
		}
	};
}
