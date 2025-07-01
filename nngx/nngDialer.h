#pragma once

#include "nngException.h"
#include "nngHooker.h"

namespace nng
{
    // Dialer �ࣺNNG ��������nng_dialer���� C++ RAII ��װ��
    // ��;���ṩ�� NNG �������ı�ݹ���֧�ֲ������Ĵ������������رա�ѡ�������Լ� TLS �� URL ����/��ȡ
    // ���ԣ�
    // - ʹ�� RAII ���� nng_dialer ��Դ��ȷ���Զ��ͷ�
    // - ֧���ƶ�������ƶ���ֵ�����ÿ����Ա�֤��Դ��ռ
    // - �ṩ��ʽ���ýӿ�������ѡ��
    // - �쳣��ȫ������������ʧ��ʱ�׳� Exception
    class Dialer
        : public Hooker<Dialer> {
    public:
        // ���캯����Ϊָ���׽��ֺ͵�ַ����������
        // ������socket - NNG �׽��֣�addr - ���ӵ�ַ
        // �쳣��������ʧ�ܣ��׳� Exception
        Dialer(nng_socket socket, std::string_view addr) noexcept(false) {
            int rv = nng_dialer_create(&_My_dialer, socket, Hooker<Dialer>::_Pre_address(addr).c_str());
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_dialer_create");
            }
        }

        // �����������رղ��ͷŲ�������Դ
        virtual ~Dialer() noexcept {
            close();
        }

        // �ƶ����캯����ת�Ʋ���������Ȩ
        // ������other - Դ Dialer ����
        Dialer(Dialer&& other) noexcept : _My_dialer(other._My_dialer) {
            other._My_dialer.id = 0;
        }

        // �ƶ���ֵ�������ת�Ʋ���������Ȩ
        // ������other - Դ Dialer ����
        // ���أ���ǰ���������
        Dialer& operator=(Dialer&& other) noexcept {
            if (this != &other) {
                close();
                _My_dialer = other._My_dialer;
                other._My_dialer.id = 0;
            }
            return *this;
        }

        // ���ÿ������캯��
        Dialer(const Dialer&) = delete;

        // ���ÿ�����ֵ�����
        Dialer& operator=(const Dialer&) = delete;

        // ����������
        // ������flags - ������־��Ĭ��Ϊ 0
        // ���أ����������0 ��ʾ�ɹ�
        int start(int flags = 0) noexcept {
            _Pre_start(*this);
            return nng_dialer_start(_My_dialer, flags);
        }

        // �رղ�����
        void close() noexcept {
            if (valid()) {
                nng_dialer_close(_My_dialer);
                _My_dialer.id = 0;
            }
        }

        // ��鲦�����Ƿ���Ч
        // ���أ�true ��ʾ��Ч��false ��ʾ��Ч
        bool valid() const noexcept {
            return _My_dialer.id != 0;
        }

        // ��ȡ������ ID
        // ���أ��������� ID
        int id() const noexcept {
            return nng_dialer_id(_My_dialer);
        }

        // ���ò����Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Dialer& set_bool(std::string_view opt, bool val) noexcept {
            nng_dialer_set_bool(_My_dialer, opt.data(), val);
            return *this;
        }

        // �������Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Dialer& set_int(std::string_view opt, int val) noexcept {
            nng_dialer_set_int(_My_dialer, opt.data(), val);
            return *this;
        }

        // ���ô�С�Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Dialer& set_size(std::string_view opt, size_t val) noexcept {
            nng_dialer_set_size(_My_dialer, opt.data(), val);
            return *this;
        }

        // ���� 64 λ�޷������Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Dialer& set_uint64(std::string_view opt, uint64_t val) noexcept {
            nng_dialer_set_uint64(_My_dialer, opt.data(), val);
            return *this;
        }

        // �����ַ����Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Dialer& set_string(std::string_view opt, std::string_view val) noexcept {
            nng_dialer_set_string(_My_dialer, opt.data(), val.data());
            return *this;
        }

        // ����ʱ���Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ѡ��ֵ�����룩
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Dialer& set_ms(std::string_view opt, nng_duration val) noexcept {
            nng_dialer_set_ms(_My_dialer, opt.data(), val);
            return *this;
        }

        // ���õ�ַ�Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - ��ֵַ
        // ���أ���ǰ��������ã�֧����ʽ���ã�
        Dialer& set_addr(std::string_view opt, const nng_sockaddr& val) noexcept {
            nng_dialer_set_addr(_My_dialer, opt.data(), &val);
            return *this;
        }

        // ��ȡ�����Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_bool(std::string_view opt, bool& val) const noexcept {
            return nng_dialer_get_bool(_My_dialer, opt.data(), &val);
        }

        // ��ȡ���Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_int(std::string_view opt, int& val) const noexcept {
            return nng_dialer_get_int(_My_dialer, opt.data(), &val);
        }

        // ��ȡ��С�Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_size(std::string_view opt, size_t& val) const noexcept {
            return nng_dialer_get_size(_My_dialer, opt.data(), &val);
        }

        // ��ȡ 64 λ�޷������Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_uint64(std::string_view opt, uint64_t& val) const noexcept {
            return nng_dialer_get_uint64(_My_dialer, opt.data(), &val);
        }

        // ��ȡ�ַ����Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ���ַ���
        // ���أ����������0 ��ʾ�ɹ�
        int get_string(std::string_view opt, std::string& val) const noexcept {
            char* out = nullptr;
            int rv = nng_dialer_get_string(_My_dialer, opt.data(), &out);
            if (rv == 0 && out != nullptr) {
                val = out;
                nng_strfree(out);
            }
            return rv;
        }

        // ��ȡʱ���Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_ms(std::string_view opt, nng_duration& val) const noexcept {
            return nng_dialer_get_ms(_My_dialer, opt.data(), &val);
        }

        // ��ȡ��ַ�Ͳ�����ѡ��
        // ������opt - ѡ�����ƣ�val - �洢��ֵַ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_addr(std::string_view opt, nng_sockaddr& val) const noexcept {
            return nng_dialer_get_addr(_My_dialer, opt.data(), &val);
        }

        // ��ȡ�������� URL
        // ������url - �洢 URL ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_url(const nng_url** url) const noexcept {
            return nng_dialer_get_url(_My_dialer, url);
        }

        // ת��Ϊ nng_dialer
        // ���أ���ǰ����� nng_dialer
        operator nng_dialer() const noexcept {
            return _My_dialer;
        }

    private:
        nng_dialer _My_dialer = NNG_DIALER_INITIALIZER;
    };
}