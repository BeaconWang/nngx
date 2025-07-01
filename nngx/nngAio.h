#pragma once

#include "nngException.h"
#include "nngMsg.h"

namespace nng
{
    // Aio �ࣺNNG �첽 I/O��nng_aio���� C++ RAII ��װ��
    // ��;���ṩ�� NNG �첽 I/O �����ı�ݹ���֧��������Ϣ������/�����������ʱ��ȡ���͵ȴ��Ȳ���
    // ���ԣ�
    // - ʹ�� RAII ���� nng_aio ��Դ��ȷ���Զ��ͷ�
    // - ֧���ƶ�������ƶ���ֵ�����ÿ����Ա�֤��Դ��ռ
    // - �ṩ�첽���������úͲ�ѯ����
    // - �쳣��ȫ������ʧ��ʱ�׳� Exception
    class Aio {
    public:
        using _Callback_t = void (*)(void*);

        // ���캯������ʼ���첽 I/O ����
        // ������callback - �ص�����ָ�룬callback_context - �ص�������ָ��
        // �쳣��������ʧ�ܣ��׳� Exception
        Aio(_Callback_t callback = nullptr, void* callback_context = nullptr) noexcept(false) {
            int rv = nng_aio_alloc(&_My_aio, callback, callback_context);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_aio_alloc");
            }
        }

        // �����������ͷ��첽 I/O ��Դ
        virtual ~Aio() noexcept {
            if (_My_aio) {
                nng_aio_free(_My_aio);
            }
        }

        // ���ÿ������캯��
        Aio(const Aio&) = delete;

        // ���ÿ�����ֵ�����
        Aio& operator=(const Aio&) = delete;

        // �ƶ����캯����ת���첽 I/O ����Ȩ
        // ������other - Դ Aio ����
        Aio(Aio&& other) noexcept : _My_aio(other._My_aio) {
            other._My_aio = nullptr;
        }

        // �ƶ���ֵ�������ת���첽 I/O ����Ȩ
        // ������other - Դ Aio ����
        // ���أ���ǰ���������
        Aio& operator=(Aio&& other) noexcept {
            if (this != &other) {
                if (_My_aio) {
                    nng_aio_free(_My_aio);
                }
                _My_aio = other._My_aio;
                other._My_aio = nullptr;
            }
            return *this;
        }

        // �����첽 I/O ����Ϣ
        // ������msg - Ҫ���õ� nng_msg ָ��
        void set_msg(nng_msg* msg) noexcept {
            nng_aio_set_msg(_My_aio, msg);
        }

        // �����첽 I/O ����Ϣ��ͨ���ƶ� Msg ����
        // ������msg - Ҫ���õ� Msg ����
        void set_msg(Msg&& msg) noexcept {
            nng_aio_set_msg(_My_aio, msg.release());
        }

        // ��ȡ�첽 I/O ����Ϣ
        // ���أ�������ǰ��Ϣ�� Msg ����
        Msg get_msg() const noexcept {
            return Msg(nng_aio_get_msg(_My_aio));
        }

        // �ͷ��첽 I/O ����Ϣ
        // ���أ������ͷ���Ϣ�� Msg ����
        Msg release_msg() const noexcept {
            auto m = nng_aio_get_msg(_My_aio);
            nng_aio_set_msg(_My_aio, nullptr);
            return Msg(m);
        }

        // �����첽 I/O �ĳ�ʱʱ��
        // ������timeout - ��ʱʱ�䣨���룩
        void set_timeout(nng_duration timeout) noexcept {
            nng_aio_set_timeout(_My_aio, timeout);
        }

        // �����첽 I/O �ľ��Թ���ʱ��
        // ������expire - ���Թ���ʱ��
        void set_expire(nng_time expire) noexcept {
            nng_aio_set_expire(_My_aio, expire);
        }

        // �����첽 I/O �� I/O ����
        // ������cnt - I/O ����������iov - I/O ��������
        // ���أ����������0 ��ʾ�ɹ�
        int set_iov(unsigned cnt, const nng_iov* iov) noexcept {
            return nng_aio_set_iov(_My_aio, cnt, iov);
        }

        // �����첽 I/O ���������
        // ������index - �������������data - �����������
        // ���أ����������0 ��ʾ�ɹ�
        int set_input(unsigned index, void* data) noexcept {
            return nng_aio_set_input(_My_aio, index, data);
        }

        // ��ȡ�첽 I/O ���������
        // ������index - �����������
        // ���أ������������ָ��
        void* get_input(unsigned index) const noexcept {
            return nng_aio_get_input(_My_aio, index);
        }

        // �����첽 I/O ���������
        // ������index - �������������data - �����������
        // ���أ����������0 ��ʾ�ɹ�
        int set_output(unsigned index, void* data) noexcept {
            return nng_aio_set_output(_My_aio, index, data);
        }

        // ��ȡ�첽 I/O ���������
        // ������index - �����������
        // ���أ������������ָ��
        void* get_output(unsigned index) const noexcept {
            return nng_aio_get_output(_My_aio, index);
        }

        // ȡ���첽 I/O ����
        void cancel() noexcept {
            nng_aio_cancel(_My_aio);
        }

        // ��ֹ�첽 I/O ����
        // ������err - ��ֹ�����Ĵ�����
        void abort(nng_err err) noexcept {
            nng_aio_abort(_My_aio, err);
        }

        // �ȴ��첽 I/O �������
        void wait() noexcept {
            nng_aio_wait(_My_aio);
        }

        // ִ���첽˯�߲���
        // ������ms - ˯��ʱ�䣨���룩
        void sleep(nng_duration ms) noexcept {
            nng_sleep_aio(ms, _My_aio);
        }

        // ����첽 I/O �Ƿ�æµ
        // ���أ�true ��ʾæµ��false ��ʾ����
        bool is_busy() const noexcept {
            return nng_aio_busy(_My_aio);
        }

        // ��ȡ�첽 I/O �����Ľ��
        // ���أ���������Ĵ�����
        nng_err result() const noexcept {
            return nng_aio_result(_My_aio);
        }

        // ��ȡ�첽 I/O �ļ���
        // ���أ������ļ���
        size_t count() const noexcept {
            return nng_aio_count(_My_aio);
        }

        // ת��Ϊ nng_aio ָ��
        // ���أ���ǰ����� nng_aio ָ��
        operator nng_aio* () const noexcept {
            return _My_aio;
        }

    private:
        nng_aio* _My_aio = nullptr;
    };
}