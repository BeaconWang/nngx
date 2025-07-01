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
	// ��ʼ�� NNG ��
	// ���أ����������0 ��ʾ�ɹ�
	inline int init() noexcept {
		return NNG_OK;
	}

	// �ͷ� NNG ����Դ
	inline void fini() noexcept {
		nng_fini();
	}
}

namespace nng
{
	// AsyncContext �ࣺ�첽�����Ļ��࣬�̳� Socket���ṩ�첽��Ϣ����֧��
	// ��;�������첽 I/O �������ṩ�̰߳�ȫ����Ϣ���ͻ���
	// ���ԣ�
	// - ʹ�� RAII ���� Aio ����
	// - ͨ�� std::recursive_mutex ȷ���̰߳�ȫ
	// - ֧���첽��Ϣ����
	class AsyncContext : virtual public Socket
	{
	protected:
		using _Ty_mutex = ::std::recursive_mutex;
		using _Ty_scoped_lock = ::std::scoped_lock<_Ty_mutex>;
		using _Ty_unique_lock = ::std::unique_lock<_Ty_mutex>;

	public:
		// ���캯������ʼ���첽������
		// ������callback - �ص�������callback_context - �ص�������
		// �쳣���� Aio ����ʧ�ܣ��׳� Exception
		AsyncContext(Aio::_Callback_t callback, void* callback_context) noexcept(false)
			: _My_aio(callback, callback_context) {
		}

		// �����������ͷ��첽��������Դ
		virtual ~AsyncContext() noexcept = default;

	protected:
		// ������Ϣ
		// ������msg - Ҫ���͵� Msg ����
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

	// AsyncSender �ࣺ�첽���ͻ��࣬�̳� AsyncContext���ṩ��Ϣ���кͻص�����
	// ��;��֧���첽��Ϣ���ͣ�������Ϣ���в�������/���ջص�
	// ���ԣ�
	// - ʹ����Ϣ���У�std::queue�������������Ϣ
	// - �ṩ�麯���ӿ���֧�������Զ��巢��/������Ϊ
	// - �̰߳�ȫ��ͨ�� AsyncContext �Ļ���������
	class AsyncSender : public AsyncContext
	{
	protected:
		typedef struct _MSG_ITEM
		{
			Msg _Msg;
			std::optional<std::promise<Msg>> _Promise_reply;
		} MSG_ITEM, * PMSG_ITEM;

	public:
		// ���캯������ʼ���첽������
		// �쳣���� Aio ����ʧ�ܣ��׳� Exception
		AsyncSender() noexcept(false) : AsyncContext(_Callback_sender, this) {}

		// �����������ͷ��첽��������Դ
		virtual ~AsyncSender() noexcept = default;

	protected:
		// ������Ϣ��
		// ������_Msg_item - ������Ϣ�Ϳ�ѡ�ظ���ŵ����Ϣ��
		void _Send(MSG_ITEM&& _Msg_item) noexcept {
			_Ty_scoped_lock locker(_My_mtx);
			_My_msgs.push(std::move(_Msg_item));

			if (_My_aio_state == INIT || _My_aio_state == IDLE) {
				AsyncContext::_Send(std::move(_My_msgs.front()._Msg));
			}
		}

	private:
		// �ص������������첽 I/O �����Ľ��
		// ������callback_context - �ص������ģ�ָ�� AsyncSender��
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

		// ���Ͷ����е���һ����Ϣ
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
		// �麯����������Ϣ�������
		// ������_Msg_item - ���͵���Ϣ��
		virtual void _On_sender_sent(MSG_ITEM& _Msg_item) noexcept {}

		// �麯�����������쳣
		// ������_Msg_item - ���͵���Ϣ�e - ������
		virtual void _On_sender_exception(MSG_ITEM& _Msg_item, nng_err e) noexcept {}

		// �麯����������յ��Ļظ���Ϣ
		// ������_Msg_item - ���͵���Ϣ�m - ���յĻظ���Ϣ
		virtual void _On_sender_recv(MSG_ITEM& _Msg_item, Msg& m) noexcept {}

	protected:
		std::queue<MSG_ITEM> _My_msgs;
	};

	// AsyncSenderNoReturn �ࣺ�޷��ص��첽���������̳� AsyncSender
	// ��;���ṩ�޻ظ�������첽��Ϣ���͹���
	// ���ԣ�
	// - ֧�ֶ�����Ϣ��ʽ��iov��Msg��������� Msg��
	// - �̳� AsyncSender ���̰߳�ȫ�Ͷ��й���
	class AsyncSenderNoReturn : public AsyncSender
	{
	public:
		// �첽���� I/O ��������
		// ������iov - I/O ����
		void async_send(const nng_iov& iov) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			_Send(std::move(mi));
		}

		// �첽������Ϣ
		// ������msg - Ҫ���͵� Msg ����
		void async_send(Msg&& msg) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			_Send(std::move(mi));
		}

		// �첽���ʹ���Ϣ����� I/O ��������
		// ������code - ��Ϣ���룬iov - I/O ����
		void async_send(Msg::_Ty_msg_code code, const nng_iov& iov) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			Msg::_Append_msg_code(mi._Msg, code);
			_Send(std::move(mi));
		}

		// �첽���ʹ���Ϣ�������Ϣ
		// ������code - ��Ϣ���룬msg - Ҫ���͵� Msg ����
		void async_send(Msg::_Ty_msg_code code, Msg&& msg) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			Msg::_Append_msg_code(mi._Msg, code);
			_Send(std::move(mi));
		}
	};

	// AsyncSenderWithReturn �ࣺ�����ص��첽���������̳� AsyncSender
	// ��;���ṩ��Ҫ�ظ����첽��Ϣ���͹��ܣ�֧�� std::future/promise
	// ���ԣ�
	// - ֧���첽��Ϣ���Ͳ�ͨ�� std::future ��ȡ�ظ�
	// - �̳� AsyncSender ���̰߳�ȫ�Ͷ��й���
	class AsyncSenderWithReturn : public AsyncSender
	{
	public:
		// �첽���� I/O �������ݲ��ȴ��ظ�
		// ������iov - I/O ������promise - ���ڴ洢�ظ��ĳ�ŵ
		void async_send(const nng_iov& iov, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// �첽������Ϣ���ȴ��ظ�
		// ������msg - Ҫ���͵� Msg ����promise - ���ڴ洢�ظ��ĳ�ŵ
		void async_send(Msg&& msg, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// �첽���ʹ���Ϣ����� I/O �������ݲ��ȴ��ظ�
		// ������code - ��Ϣ���룬iov - I/O ������promise - ���ڴ洢�ظ��ĳ�ŵ
		void async_send(Msg::_Ty_msg_code code, const nng_iov& iov, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = Msg(iov);
			Msg::_Append_msg_code(mi._Msg, code);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// �첽���ʹ���Ϣ�������Ϣ���ȴ��ظ�
		// ������code - ��Ϣ���룬msg - Ҫ���͵� Msg ����promise - ���ڴ洢�ظ��ĳ�ŵ
		void async_send(Msg::_Ty_msg_code code, Msg&& msg, std::promise<Msg>&& promise) noexcept {
			MSG_ITEM mi;
			mi._Msg = std::move(msg);
			Msg::_Append_msg_code(mi._Msg, code);
			mi._Promise_reply = std::move(promise);
			_Send(std::move(mi));
		}

		// �첽������Ϣ������ future
		// ������msg - Ҫ���͵� Msg ����
		// ���أ�std::future ���ڻ�ȡ�ظ���Ϣ
		std::future<Msg> async_send(Msg&& msg) noexcept(false) {
			std::promise<Msg> promise;
			auto future = promise.get_future();
			async_send(std::move(msg), std::move(promise));
			return future;
		}

		// �첽���ʹ���Ϣ�������Ϣ������ future
		// ������code - ��Ϣ���룬msg - Ҫ���͵� Msg ����
		// ���أ�std::future ���ڻ�ȡ�ظ���Ϣ
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
    // Dispatcher �ࣺ��Ϣ�ַ����࣬�ṩ��Ϣ�����麯���ӿ�
    // ��;��������Ϣ����ص�����������ʵ�־���ַ��߼�
    // ���ԣ�
    // - �ṩ����ԭʼ��Ϣ�ͱ�����Ϣ���麯��
    // - ֧���쳣����ص�
    class Dispatcher
    {
    protected:
        // �麯��������ԭʼ��Ϣ
        // ������msg - ���յ� Msg ����
        // ���أ�true ��ʾ��Ϣ�Ѵ���false ��ʾ��������
        virtual bool _On_raw_message(Msg& msg) { return false; }

        // �麯�������������Ϣ
        // ������code - ��Ϣ���룬msg - ���յ� Msg ����
        // ���أ���Ϣ������
        virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) { return {}; }

        // �麯��������ַ��쳣
        // ������e - �쳣����
        // ���أ�true ��ʾֹͣ�ַ���false ��ʾ����
        virtual bool _On_dispatch_exception(const Exception& e) { return true; }
    };

	// DispatcherNoReturn �ࣺ�޷��ص���Ϣ�ַ������̳� Dispatcher �� Socket
	// ��;��������յ���Ϣ�����跢�ͻظ�
	// ���ԣ�
	// - ѭ�����ղ�������Ϣ
	// - ֧���쳣����
	class DispatcherNoReturn : public Dispatcher, virtual public Socket
	{
	public:
		// �ַ���Ϣ
		// ѭ��������Ϣ�����ô���ص���ֱ���쳣��ֹ
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
		// �麯����������յ�����Ϣ
		// ������m - ���յ� Msg ����
		// ���أ�true ��ʾ��Ϣ�Ѵ���false ��ʾ��������
		virtual bool _On_receiver_recv(Msg& m) noexcept {
			if (!_On_raw_message(m)) {
				auto code = Msg::_Chop_msg_code(m);
				_On_message(code, m);
			}
			return false;
		}
	};

	// DispatcherWithReturn �ࣺ�����ص���Ϣ�ַ������̳� Dispatcher �� Socket
	// ��;��������յ���Ϣ�����ͻظ�
	// ���ԣ�
	// - ѭ�����ա�������Ϣ�����ͻظ�
	// - ֧���쳣����
	class DispatcherWithReturn : public Dispatcher, virtual public Socket
	{
	public:
		// �ַ���Ϣ
		// ѭ��������Ϣ���������ͻظ���ֱ���쳣��ֹ
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
		// �麯����������յ�����Ϣ
		// ������m - ���յ� Msg ����
		// ���أ�true ��ʾ��Ϣ�Ѵ���false ��ʾ��������
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
	// Pull �ࣺPull Э���ģ���࣬�̳� Peer��Socket �� DispatcherNoReturn
	// ��;��ʵ�� Pull Э����첽��Ϣ����
	// ���ԣ�
	// - ֧�� Listener �� Dialer ������
	// - �ṩ�޷��ص���Ϣ�ַ�
	template <class _Connector_t = Listener>
	class Pull : public Peer<_Connector_t>, virtual public Socket, public DispatcherNoReturn
	{
		// ���� Pull Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_pull0_open);
		}
	};

	// Push �ࣺPush Э���ģ���࣬�̳� Peer��Socket �� AsyncSenderNoReturn
	// ��;��ʵ�� Push Э����첽��Ϣ����
	// ���ԣ�
	// - ֧�� Listener �� Dialer ������
	// - �ṩ�޷��ص��첽����
	template <class _Connector_t = Dialer>
	class Push : public Peer<_Connector_t>, virtual public Socket, public AsyncSenderNoReturn
	{
		// ���� Push Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_push0_open);
		}
	};
}

namespace nng
{
	// Pair �ࣺPair Э���ģ���࣬�̳� Peer��Socket��DispatcherNoReturn �� AsyncSenderNoReturn
	// ��;��ʵ�� Pair Э����첽��Ϣ���ͺͽ���
	// ���ԣ�
	// - ֧�� Listener �� Dialer ������
	// - �ṩ�޷��صķ��ͺͷַ�
	template <class _Connector_t>
	class Pair : public Peer<_Connector_t>, virtual public Socket, public DispatcherNoReturn, public AsyncSenderNoReturn
	{
		// ���� Pair Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_pair0_open);
		}
	};
}

namespace nng
{
	// Response �ࣺRep Э���࣬�̳� Peer �� DispatcherWithReturn
	// ��;��ʵ�� Rep Э�����Ϣ���պͻظ�
	// ���ԣ�
	// - ʹ�� Listener ������
	// - �ṩ�����ص���Ϣ�ַ�
	class Response : public Peer<Listener>, virtual public Socket, public DispatcherWithReturn
	{
		// ���� Rep Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_rep0_open);
		}
	};

	// Request �ࣺReq Э���࣬�̳� Peer �� AsyncSenderWithReturn
	// ��;��ʵ�� Req Э����첽��Ϣ���ͺͻظ�����
	// ���ԣ�
	// - ʹ�� Dialer ������
	// - �ṩ�����ص��첽����
	class Request : public Peer<Dialer>, virtual public Socket, public AsyncSenderWithReturn
	{
		// ���� Req Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_req0_open);
		}

	public:
		// �򵥷�����Ϣ�����ջظ�
		// ������addr - ���ӵ�ַ��msg - ��Ϣ����pre_send - ����ǰ�ص�
		// ���أ����������0 ��ʾ�ɹ�
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

		// �򵥷��ʹ���Ϣ�������Ϣ�����ս��
		// ������addr - ���ӵ�ַ��code - ��Ϣ���룬msg - ��Ϣ����pre_send - ����ǰ�ص�
		// ���أ���Ϣ������
		// �쳣������������ʧ�ܣ��׳� Exception
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
	// Publisher �ࣺPub Э���ģ���࣬�̳� Peer �� AsyncSenderNoReturn
	// ��;��ʵ�� Pub Э����첽��Ϣ����
	// ���ԣ�
	// - ֧�� Listener �� Dialer ������
	// - �ṩ�޷��ص��첽����
	template <class _Connector_t = Listener>
	class Publisher : public Peer<_Connector_t>, virtual public Socket, public AsyncSenderNoReturn
	{
		// ���� Pub Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_pub0_open);
		}
	};

	// Subscriber �ࣺSub Э���ģ���࣬�̳� Peer �� DispatcherNoReturn
	// ��;��ʵ�� Sub Э����첽��Ϣ����
	// ���ԣ�
	// - ֧�� Listener �� Dialer ������
	// - �ṩ�޷��ص���Ϣ�ַ�
	template <class _Connector_t = Dialer>
	class Subscriber : public Peer<_Connector_t>, virtual public Socket, public DispatcherNoReturn
	{
		// ���� Sub Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_sub0_open);
		}
	};

}

namespace nng
{
	// Survey �ࣺSurveyor Э���ģ���࣬�̳� Peer �� AsyncSenderWithReturn
	// ��;��ʵ�� Surveyor Э����첽��Ϣ���ͺͶ�ظ�����
	// ���ԣ�
	// - ֧�� Listener �� Dialer ������
	// - �ṩ�����ص��첽����
	template <class _Connector_t = Listener>
	class Survey : public Peer<_Connector_t>, virtual public Socket, public AsyncSenderWithReturn
	{
		// ���� Surveyor Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_surveyor0_open);
		}

	public:
		// ������Ϣ�����ն���ظ�
		// ������msg - Ҫ���͵� Msg ����iter - ���������ڴ洢�ظ�
		// ���أ����������0 ��ʾ�ɹ�
		// �쳣�������ճ�ʱ����Ĵ����׳� Exception
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

		// ���ʹ���Ϣ�������Ϣ�����ն���ظ�
		// ������code - ��Ϣ���룬msg - Ҫ���͵� Msg ����iter - ���������ڴ洢�ظ�
		// ���أ����������0 ��ʾ�ɹ�
		// �쳣�������ճ�ʱ����Ĵ����׳� Exception
		template <typename _Iter_t>
		int send(Msg::_Ty_msg_code code, Msg&& msg, _Iter_t iter) noexcept(false) {
			Msg::_Append_msg_code(msg, code);
			return send(std::move(msg), iter);
		}
	};

	// Respond �ࣺRespondent Э���ģ���࣬�̳� Peer �� DispatcherWithReturn
	// ��;��ʵ�� Respondent Э�����Ϣ���պͻظ�
	// ���ԣ�
	// - ֧�� Listener �� Dialer ������
	// - �ṩ�����ص���Ϣ�ַ�
	template <class _Connector_t = Dialer>
	class Respond : public Peer<_Connector_t>, virtual public Socket, public DispatcherWithReturn
	{
		// ���� Respondent Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_respondent0_open);
		}
	};
}

namespace nng
{
	// Bus �ࣺBus Э���࣬�̳� Peer��Socket��DispatcherNoReturn �� AsyncSenderNoReturn
	// ��;��ʵ�� Bus Э����첽��Ϣ���ͺͽ���
	// ���ԣ�
	// - ʹ�� Listener ��������֧�ֶ�̬��� Dialer
	// - �ṩ�޷��صķ��ͺͷַ�
	class Bus : public Peer<Listener>, virtual public Socket, public DispatcherNoReturn, public AsyncSenderNoReturn
	{
		// ���� Bus Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
		virtual int _Create() noexcept override {
			return Socket::create(nng_bus0_open);
		}

	public:
		// ��Ӳ�����������
		// ������addr - ���ӵ�ַ
		// ���أ����������0 ��ʾ�ɹ�
		// �쳣��������������ʧ�ܣ��׳� Exception
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
	// ResponseParallel �ࣺ���� Rep Э���࣬�̳� Peer �� DispatcherWithReturn
	// ��;��ʵ�� Rep Э��Ĳ�����Ϣ����֧�ֶ��������
	// ���ԣ�
	// - ʹ�� Listener ������
	// - ͨ����������WORK_ITEM�����д�����Ϣ
	// - �ṩ�����ص���Ϣ�ַ�
	class ResponseParallel : public Peer<Listener>, virtual public Socket, public DispatcherWithReturn
	{
		// ���� Rep Э���׽���
		// ���أ����������0 ��ʾ�ɹ�
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

			// ������캯��
			// ������socket - �׽��֣�callback - �ص�������owner - ������
			// �쳣���� Aio �� Ctx ����ʧ�ܣ��׳� Exception
			explicit _WORK_ITEM(nng_socket socket, void (*callback)(void*), void* owner) noexcept(false)
				: _Aio(callback, this), _Ctx(socket), _Owner(owner) {
			}

			// �첽������Ϣ
			// ������msg - Ҫ���͵� Msg ����
			inline void send() noexcept {
				send(std::move(_Msg));
			}

			// �첽������Ϣ
			// ������msg - Ҫ���͵� Msg ����
			inline void send(Msg&& msg) noexcept {
				_Aio.set_msg(std::move(msg));
				_State = WIS_SEND;
				_Ctx.send(_Aio);
			}

			// �첽�ȴ�
			// ������ms - �ȴ�ʱ�䣨���룩
			inline void wait(nng_duration ms) noexcept {
				_State = WIS_WAIT;
				_Aio.sleep(ms);
			}
		} WORK_ITEM, * PWORK_ITEM;

	public:
		// �������д���
		// ������addr - ������ַ��parallel - ���й�����������flags - ������־
		// ���أ����������0 ��ʾ�ɹ�
		// �쳣��������������ʧ�ܣ��׳� Exception
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
		// �ص�����������������첽����
		// ������arg - ������ָ��
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
		// �麯������������ر�
		// ������_Work_item - ������ָ��
		virtual void _On_close(PWORK_ITEM _Work_item) {}

		// �麯��������ԭʼ��Ϣ
		// ������msg - ���յ� Msg ����
		// ���أ�true ��ʾ��Ϣ�Ѵ���false ��ʾ��������
		virtual bool _On_raw_message(Msg& msg) override { return false; }

		// �麯��������ԭʼ��Ϣ�����õȴ�ʱ��
		// ������msg - ���յ� Msg ����_Wait_ms - �ȴ�ʱ��
		// ���أ�true ��ʾ��Ϣ�Ѵ���false ��ʾ��������
		virtual bool _On_raw_message(Msg& msg, nng_duration& _Wait_ms) {
			return _On_raw_message(msg);
		}

		// �麯�������������Ϣ
		// ������code - ��Ϣ���룬msg - ���յ� Msg ����
		// ���أ���Ϣ������
		virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
			return {};
		}

		// �麯�������������Ϣ�����õȴ�ʱ��
		// ������code - ��Ϣ���룬msg - ���յ� Msg ����_Wait_ms - �ȴ�ʱ��
		// ���أ���Ϣ������
		virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg, nng_duration& _Wait_ms) {
			return _On_message(code, msg);
		}

		// �麯��������ȴ�״̬
		// ������_Wait_ms - �ȴ�ʱ��
		virtual void _On_wait(nng_duration& _Wait_ms) {}

		// ����ȴ�״̬�Ĺ�����
		// ������_Work_item - ������ָ��
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

		// ������յ�����Ϣ
		// ������_Work_item - ������ָ��
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

		// �麯���������쳣
		// ������e - ������
		virtual void _On_exception(nng_err e) {}

	protected:
		std::vector<WORK_ITEM> _My_work_items;
	};
}