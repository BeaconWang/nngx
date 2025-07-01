#pragma once

#include "nngException.h"
#include "nngMsg.h"

namespace nng
{
    // Socket �ࣺNNG �׽��֣�nng_socket���� C++ RAII ��װ��
    // ��;���ṩ�� NNG �׽��ֵı�ݹ���֧���׽��ִ������رա���Ϣ����/���ա�ѡ�����ü��ض�Э�����
    // ���ԣ�
    // - ʹ�� RAII ���� nng_socket ��Դ��ȷ���Զ��ͷ�
    // - ֧���ƶ�������ƶ���ֵ�����ÿ����Ա�֤��Դ��ռ
    // - �ṩ��ʽ���ýӿ�������ѡ��
    // - �ṩ��̬���������Դ����ض�Э����׽��֣��� req��rep��pub �ȣ�
    // - �쳣��ȫ���׽��ִ����򲿷ַ���/���ղ���ʧ��ʱ�׳� Exception
    class Socket {
    public:
        using _Socket_creator_t = int (*)(nng_socket*);

    public:
        // Ĭ�Ϲ��캯��������δ��ʼ�����׽���
        explicit Socket() noexcept = default;

        // ���캯����ʹ������ nng_socket ��ʼ���׽���
        // ������socket - NNG �׽���
        explicit Socket(nng_socket socket) noexcept : _My_socket(socket) {}

        // ���캯����ʹ��ָ���Ĵ���������ʼ���׽���
        // ������creator - �׽��ִ�������
        // �쳣��������ʧ�ܣ��׳� Exception
        explicit Socket(_Socket_creator_t creator) noexcept(false) {
            auto rv = creator(&_My_socket);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_xxx_open");
            }
        }

        // �����������رղ��ͷ��׽�����Դ
        virtual ~Socket() noexcept {
            close();
        }

        // �ƶ����캯����ת���׽�������Ȩ
        // ������other - Դ Socket ����
        Socket(Socket&& other) noexcept : _My_socket(other._My_socket) {
            other._My_socket.id = 0;
        }

        // �ƶ���ֵ�������ת���׽�������Ȩ
        // ������other - Դ Socket ����
        // ���أ���ǰ���������
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                close();
                _My_socket = other._My_socket;
                other._My_socket.id = 0;
            }
            return *this;
        }

        // ���ÿ������캯��
        Socket(const Socket&) = delete;

        // ���ÿ�����ֵ�����
        Socket& operator=(const Socket&) = delete;

        // �����׽���
        // ������creator - �׽��ִ�������
        // ���أ����������0 ��ʾ�ɹ�
        int create(_Socket_creator_t creator) noexcept {
            assert(creator);
            assert(_My_socket.id == 0);
            return creator(&_My_socket);
        }

        // �ر��׽���
        void close() noexcept {
            if (valid()) {
                nng_socket_close(_My_socket);
                _My_socket.id = 0;
            }
        }

        // ����׽����Ƿ���Ч
        // ���أ�true ��ʾ��Ч��false ��ʾ��Ч
        bool valid() const noexcept {
            return _My_socket.id != 0;
        }

        // ��ȡ�׽��� ID
        // ���أ��׽��ֵ� ID
        int id() const noexcept {
            return nng_socket_id(_My_socket);
        }

        // ��̬�������������� req Э���׽���
        // ���أ�req Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket req() noexcept(false) { return _Create(nng_req0_open); }

        // ��̬�������������� rep Э���׽���
        // ���أ�rep Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket rep() noexcept(false) { return _Create(nng_rep0_open); }

        // ��̬�������������� pub Э���׽���
        // ���أ�pub Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket pub() noexcept(false) { return _Create(nng_pub0_open); }

        // ��̬�������������� sub Э���׽���
        // ���أ�sub Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket sub() noexcept(false) { return _Create(nng_sub0_open); }

        // ��̬�������������� push Э���׽���
        // ���أ�push Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket push() noexcept(false) { return _Create(nng_push0_open); }

        // ��̬�������������� pull Э���׽���
        // ���أ�pull Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket pull() noexcept(false) { return _Create(nng_pull0_open); }

        // ��̬�������������� surveyor Э���׽���
        // ���أ�surveyor Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket surveyor() noexcept(false) { return _Create(nng_surveyor0_open); }

        // ��̬�������������� respondent Э���׽���
        // ���أ�respondent Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket respond() noexcept(false) { return _Create(nng_respondent0_open); }

        // ��̬�������������� bus Э���׽���
        // ���أ�bus Э��� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket bus() noexcept(false) { return _Create(nng_bus0_open); }

        // ��̬��������������ԭʼģʽ�� req Э���׽���
        // ���أ�ԭʼģʽ�� req Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket req_raw() noexcept(false) { return _Create(nng_req0_open_raw); }

        // ��̬��������������ԭʼģʽ�� rep Э���׽���
        // ���أ�ԭʼģʽ�� rep Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket rep_raw() noexcept(false) { return _Create(nng_rep0_open_raw); }

        // ��̬��������������ԭʼģʽ�� pub Э���׽���
        // ���أ�ԭʼģʽ�� pub Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket pub_raw() noexcept(false) { return _Create(nng_pub0_open_raw); }

        // ��̬��������������ԭʼģʽ�� sub Э���׽���
        // ���أ�ԭʼģʽ�� sub Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket sub_raw() noexcept(false) { return _Create(nng_sub0_open_raw); }

        // 	static��������������ԭʼģʽ�� push Э���׽���
        // 	���أ�ԭʼģʽ�� push Э�� Socket ����
        // 	�쳣��������ʧ�ܣ��׳� Exception
        static Socket push_raw() noexcept(false) { return _Create(nng_push0_open_raw); }

        // ��̬��������������ԭʼģʽ�� pull Э���׽���
        // ���أ�ԭʼģʽ�� pull Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket pull_raw() noexcept(false) { return _Create(nng_pull0_open_raw); }

        // ��̬��������������ԭʼģʽ�� surveyor Э���׽���
        // ���أ�ԭʼģʽ�� surveyor Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket surveyor_raw() noexcept(false) { return _Create(nng_surveyor0_open_raw); }

        // ��̬��������������ԭʼģʽ�� respondent Э���׽���
        // ���أ�ԭʼģʽ�� respondent Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket respond_raw() noexcept(false) { return _Create(nng_respondent0_open_raw); }

        // ��̬��������������ԭʼģʽ�� bus Э���׽���
        // ���أ�ԭʼģʽ�� bus Э�� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket bus_raw() noexcept(false) { return _Create(nng_bus0_open_raw); }

        // ���ò������׽���ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Socket& set_bool(std::string_view opt, bool val) noexcept {
            nng_socket_set_bool(_My_socket, opt.data(), val);
            return *this;
        }

        // ���������׽���ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Socket& set_int(std::string_view opt, int val) noexcept {
            nng_socket_set_int(_My_socket, opt.data(), val);
            return *this;
        }

        // ���ô�С���׽���ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Socket& set_size(std::string_view opt, size_t val) noexcept {
            nng_socket_set_size(_My_socket, opt.data(), val);
            return *this;
        }

        // ����ʱ�����׽���ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ�����룩
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Socket& set_ms(std::string_view opt, nng_duration val) noexcept {
            nng_socket_set_ms(_My_socket, opt.data(), val);
            return *this;
        }

        // ��ȡ�������׽���ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_bool(std::string_view opt, bool& val) const noexcept {
            return nng_socket_get_bool(_My_socket, opt.data(), &val);
        }

        // ��ȡ�����׽���ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_int(std::string_view opt, int& val) const noexcept {
            return nng_socket_get_int(_My_socket, opt.data(), &val);
        }

        // ��ȡ��С���׽���ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_size(std::string_view opt, size_t& val) const noexcept {
            return nng_socket_get_size(_My_socket, opt.data(), &val);
        }

        // ��ȡʱ�����׽���ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_ms(std::string_view opt, nng_duration& val) const noexcept {
            return nng_socket_get_ms(_My_socket, opt.data(), &val);
        }

        // ��ȡЭ�� ID
        // ������id - �洢Э�� ID ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_proto_id(uint16_t& id) const noexcept {
            return nng_socket_proto_id(_My_socket, &id);
        }

        // ��ȡ�Զ�Э�� ID
        // ������id - �洢�Զ�Э�� ID ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_peer_id(uint16_t& id) const noexcept {
            return nng_socket_peer_id(_My_socket, &id);
        }

        // ��ȡЭ������
        // ������name - �洢Э�����Ƶ�ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_proto_name(const char** name) const noexcept {
            return nng_socket_proto_name(_My_socket, name);
        }

        // ��ȡ�Զ�Э������
        // ������name - �洢�Զ�Э�����Ƶ�ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_peer_name(const char** name) const noexcept {
            return nng_socket_peer_name(_My_socket, name);
        }

        // ��ȡ�׽����Ƿ�Ϊԭʼģʽ
        // ������raw - �洢ԭʼģʽ��־��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_raw(bool& raw) const noexcept {
            return nng_socket_raw(_My_socket, &raw);
        }

        // ��ʽ���׽��ֵ�ַ
        // ������sa - �׽��ֵ�ַ��buf - �洢��ʽ������Ļ�������bufsz - ��������С
        // ���أ���ʽ����ĵ�ַ�ַ���
        static const char* format_sockaddr(const nng_sockaddr& sa, char* buf, size_t bufsz) noexcept {
            return nng_str_sockaddr(&sa, buf, bufsz);
        }

        // SUB0 Э�飺����ָ������
        // ������sv - �����ַ�����Ĭ��Ϊ��
        // ���أ����������0 ��ʾ�ɹ�
        int socket_subscribe(std::string_view sv = "") noexcept {
            return nng_sub0_socket_subscribe(_My_socket, sv.data(), sv.length());
        }

        // SUB0 Э�飺ȡ������ָ������
        // ������sv - �����ַ�����Ĭ��Ϊ��
        // ���أ����������0 ��ʾ�ɹ�
        int socket_unsubscribe(std::string_view sv = "") noexcept {
            return nng_sub0_socket_unsubscribe(_My_socket, sv.data(), sv.length());
        }

        // �첽������Ϣ
        // ������aio - �첽 I/O ����
        void send(nng_aio* aio) noexcept {
            nng_send_aio(_My_socket, aio);
        }

        // �첽������Ϣ
        // ������aio - �첽 I/O ����
        void recv(nng_aio* aio) noexcept {
            nng_recv_aio(_My_socket, aio);
        }

        // ͬ����������
        // ������data - ����ָ�룬data_size - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int send(const void* data, size_t data_size) noexcept {
            return nng_send(_My_socket, (void*)data, data_size, 0);
        }

        // ͬ������ I/O ��������
        // ������iov - I/O ����
        // ���أ����������0 ��ʾ�ɹ�
        int send(const nng_iov& iov) noexcept {
            return nng_send(_My_socket, iov.iov_buf, iov.iov_len, 0);
        }

        // ������Ϣ�����շ��ؽ��
        // ������code - ��Ϣ���룬msg - ��Ϣ����
        // ���أ���Ϣ���
        // �쳣������Ϣ�ط������ʧ�ܣ��׳� Exception
        Msg::_Ty_msg_result send(Msg::_Ty_msg_code code, Msg& msg) noexcept(false) {
            int rv;
            if (!msg) {
                rv = msg.realloc(0);
                if (rv != NNG_OK) {
                    throw Exception(rv, "realloc");
                }
            }

            Msg::_Append_msg_code(msg, code);

            rv = nng_sendmsg(_My_socket, msg, 0);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_sendmsg");
            }

            msg = recv();

            return Msg::_Chop_msg_result(msg);
        }

        // ������Ϣ�����շ�����Ϣ
        // ������msg - ��Ϣ����
        // ���أ����������0 ��ʾ�ɹ�
        int send(Msg& msg) noexcept {
            int rv;
            if (!msg) {
                rv = msg.realloc(0);
                if (rv != NNG_OK) {
                    return rv;
                }
            }

            rv = send(std::move(msg));
            if (rv == NNG_OK) {
                msg.release();
            }

            return recv(msg);
        }

        // ������Ϣ�������շ��أ�
        // ������msg - ��Ϣ����
        // ���أ����������0 ��ʾ�ɹ�
        // ע�⣺�����ͳɹ���msg ����Դ�ᱻ�ͷ�
        int send(Msg&& msg) noexcept {
            int rv = nng_sendmsg(_My_socket, msg, 0);
            if (rv == NNG_OK) {
                msg.release();
            }
            return rv;
        }

        // ���ʹ���Ϣ�������Ϣ�������շ��أ�
        // ������code - ��Ϣ���룬msg - ��Ϣ����Ĭ��Ϊ�գ�
        // ���أ����������0 ��ʾ�ɹ�
        // ע�⣺�����ͳɹ���msg ����Դ�ᱻ�ͷ�
        int send(Msg::_Ty_msg_code code, Msg&& msg = Msg{}) noexcept {
            int rv;
            if (!msg) {
                rv = msg.realloc(0);
                if (rv != NNG_OK) {
                    return rv;
                }
            }

            Msg::_Append_msg_code(msg, code);

            rv = send(std::move(msg));
            if (rv == NNG_OK) {
                msg.release();
            }

            return rv;
        }

        // ����ԭʼ��Ϣ
        // ������msg - ԭʼ��Ϣָ��
        // ���أ����������0 ��ʾ�ɹ�
        int send(nng_msg* msg) noexcept {
            return nng_sendmsg(_My_socket, msg, 0);
        }

        // ͬ����������
        // ������data - ���ݻ�������size - ���ݴ�Сָ�룬flags - ���ձ�־
        // ���أ����������0 ��ʾ�ɹ�
        int recv(void* data, size_t* size, int flags) noexcept {
            return nng_recv(_My_socket, data, size, flags);
        }

        // ͬ������ԭʼ��Ϣ
        // ������msg - �洢��Ϣ��ָ�룬flags - ���ձ�־��Ĭ��Ϊ 0
        // ���أ����������0 ��ʾ�ɹ�
        int recv(nng_msg** msg, int flags = 0) noexcept {
            return nng_recvmsg(_My_socket, msg, flags);
        }

        // ͬ��������Ϣ
        // ������flags - ���ձ�־��Ĭ��Ϊ 0
        // ���أ����յ��� Msg ����
        // �쳣��������ʧ�ܣ��׳� Exception
        Msg recv(int flags = 0) noexcept(false) {
            nng_msg* msg = nullptr;
            int rv = nng_recvmsg(_My_socket, &msg, flags);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_recvmsg");
            }

            return Msg(msg);
        }

        // ͬ��������Ϣ��ָ������
        // ������msg - �洢������Ϣ�� Msg ����
        // ���أ����������0 ��ʾ�ɹ�
        int recv(Msg& msg) noexcept {
            nng_msg* m = nullptr;
            int rv = nng_recvmsg(_My_socket, &m, 0);
            if (rv != NNG_OK) {
                return rv;
            }

            msg = m;
            return rv;
        }

        // ת��Ϊ nng_socket
        // ���أ���ǰ����� nng_socket
        operator nng_socket() const noexcept {
            return _My_socket;
        }

    private:
        // ��̬��������������ָ��Э����׽���
        // ������creator - �׽��ִ�������
        // ���أ������� Socket ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static Socket _Create(_Socket_creator_t creator) noexcept(false) {
            nng_socket s;
            auto rv = creator(&s);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_xxx_open");
            }
            return Socket(s);
        }

        nng_socket _My_socket = NNG_SOCKET_INITIALIZER;
    };
}