#pragma once

#include "nngException.h"
#include "nngHooker.h"

namespace nng
{
    // Listener �ࣺNNG ��������nng_listener���� C++ RAII ��װ��
    // ��;���ṩ�� NNG �������ı�ݹ���֧�ּ������Ĵ������������رպ�ѡ������
    // ���ԣ�
    // - ʹ�� RAII ���� nng_listener ��Դ��ȷ���Զ��ͷ�
    // - ֧���ƶ�������ƶ���ֵ�����ÿ����Ա�֤��Դ��ռ
    // - �ṩѡ�����ýӿڣ�֧�ֶ����������͵����úͻ�ȡ
    // - �쳣��ȫ������������ʧ��ʱ�׳� Exception
    class Listener
        : public Hooker<Listener> {
    public:
        // Ĭ�Ϲ��캯��������δ��ʼ���ļ�����
        Listener() noexcept = default;

        // ���캯����Ϊָ���׽��ֺ͵�ַ����������
        // ������sock - NNG �׽��֣�addr - ������ַ
        // �쳣��������ʧ�ܣ��׳� Exception
        Listener(nng_socket sock, std::string_view addr) noexcept(false) {
            int rv = nng_listener_create(&_My_listener, sock, Hooker<Listener>::_Pre_address(addr).c_str());
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_listener_create");
            }
        }

        // �����������رղ��ͷż�������Դ
        virtual ~Listener() noexcept {
            close();
        }

        // �ƶ����캯����ת�Ƽ���������Ȩ
        // ������other - Դ Listener ����
        Listener(Listener&& other) noexcept : _My_listener(other._My_listener) {
            other._My_listener.id = 0;
        }

        // �ƶ���ֵ�������ת�Ƽ���������Ȩ
        // ������other - Դ Listener ����
        // ���أ���ǰ���������
        Listener& operator=(Listener&& other) noexcept {
            if (this != &other) {
                close();
                _My_listener = other._My_listener;
                other._My_listener.id = 0;
            }
            return *this;
        }

        // ���ÿ������캯��
        Listener(const Listener&) = delete;

        // ���ÿ�����ֵ�����
        Listener& operator=(const Listener&) = delete;

        // ����������
        // ������flags - ������־��Ĭ��Ϊ 0
        // ���أ����������0 ��ʾ�ɹ�
        int start(int flags = 0) noexcept {
            _Pre_start(*this);
            return nng_listener_start(_My_listener, flags);
        }

        // �رռ�����
        void close() noexcept {
            if (valid()) {
                nng_listener_close(_My_listener);
                _My_listener.id = 0;
            }
        }

        // ��ȡ������ ID
        // ���أ��������� ID
        int id() const noexcept {
            return nng_listener_id(_My_listener);
        }

        // ���������Ƿ���Ч
        // ���أ�true ��ʾ��Ч��false ��ʾ��Ч
        bool valid() const noexcept {
            return _My_listener.id != 0;
        }

        // ����ָ���ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ��ָ�룩
        // ���أ����������0 ��ʾ�ɹ�
        int set_ptr(std::string_view opt, void* val) noexcept {
            return nng_listener_set_ptr(_My_listener, opt.data(), val);
        }

        // ��ȡָ���ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_ptr(std::string_view opt, void** val) const noexcept {
            return nng_listener_get_ptr(_My_listener, opt.data(), val);
        }

        // ���ò����ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_bool(std::string_view opt, bool val) noexcept {
            return nng_listener_set_bool(_My_listener, opt.data(), val);
        }

        // �������ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_int(std::string_view opt, int val) noexcept {
            return nng_listener_set_int(_My_listener, opt.data(), val);
        }

        // ���ô�С�ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_size(std::string_view opt, size_t val) noexcept {
            return nng_listener_set_size(_My_listener, opt.data(), val);
        }

        // ����ʱ���ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ�����룩
        // ���أ����������0 ��ʾ�ɹ�
        int set_ms(std::string_view opt, nng_duration val) noexcept {
            return nng_listener_set_ms(_My_listener, opt.data(), val);
        }

        // ���� 64 λ�޷������ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_uint64(std::string_view opt, uint64_t val) noexcept {
            return nng_listener_set_uint64(_My_listener, opt.data(), val);
        }

        // �����ַ����ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_string(std::string_view opt, std::string_view val) noexcept {
            return nng_listener_set_string(_My_listener, opt.data(), val.data());
        }

        // ��ȡ�����ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_bool(std::string_view opt, bool& val) const noexcept {
            return nng_listener_get_bool(_My_listener, opt.data(), &val);
        }

        // ��ȡ���ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_int(std::string_view opt, int& val) const noexcept {
            return nng_listener_get_int(_My_listener, opt.data(), &val);
        }

        // ��ȡ��С�ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_size(std::string_view opt, size_t& val) const noexcept {
            return nng_listener_get_size(_My_listener, opt.data(), &val);
        }

        // ��ȡʱ���ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_ms(std::string_view opt, nng_duration& val) const noexcept {
            return nng_listener_get_ms(_My_listener, opt.data(), &val);
        }

        // ��ȡ 64 λ�޷������ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_uint64(std::string_view opt, uint64_t& val) const noexcept {
            return nng_listener_get_uint64(_My_listener, opt.data(), &val);
        }

        // ��ȡ�ַ����ͼ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ���ַ���
        // ���أ����������0 ��ʾ�ɹ�
        int get_string(std::string_view opt, std::string& val) const noexcept {
            char* out = nullptr;
            int rv = nng_listener_get_string(_My_listener, opt.data(), &out);
            if (rv == 0 && out) {
                val = out;
                nng_strfree(out);
            }
            return rv;
        }

        // ת��Ϊ nng_listener
        // ���أ���ǰ����� nng_listener
        operator nng_listener() const noexcept {
            return _My_listener;
        }

    private:
        nng_listener _My_listener = NNG_LISTENER_INITIALIZER;
    };
}