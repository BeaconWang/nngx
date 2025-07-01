#pragma once

#include <string_view>

#include "nngException.h"
#include "nngSocket.h"
#include "nngMsg.h"
#include "nngAio.h"

namespace nng
{
    // Ctx �ࣺNNG �����ģ�nng_ctx���� C++ RAII ��װ��
    // ��;���ṩ�� NNG �����ĵı�ݹ���֧����Ϣ���첽��ͬ������/���ա�ѡ�������Լ� SUB0 Э��Ķ��Ĳ���
    // ���ԣ�
    // - ʹ�� RAII ���� nng_ctx ��Դ��ȷ���Զ��ͷ�
    // - ֧���ƶ�������ƶ���ֵ�����ÿ����Ա�֤��Դ��ռ
    // - �ṩ�첽��ͬ������Ϣ������������ѡ������
    // - �쳣��ȫ�������Ĵ����������Ϣʧ��ʱ�׳� Exception
    class Ctx {
    public:
        // ���캯����Ϊָ���׽��ִ���������
        // ������socket - NNG �׽���
        // �쳣��������ʧ�ܣ��׳� Exception
        explicit Ctx(nng_socket socket) noexcept(false) {
            int rv = nng_ctx_open(&_My_ctx, socket);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_ctx_open");
            }
        }

        // ���ÿ������캯��
        Ctx(const Ctx&) = delete;

        // ���ÿ�����ֵ�����
        Ctx& operator=(const Ctx&) = delete;

        // �ƶ����캯����ת������������Ȩ
        // ������other - Դ Ctx ����
        Ctx(Ctx&& other) noexcept : _My_ctx(other._My_ctx) {
            other._My_ctx = NNG_CTX_INITIALIZER;
        }

        // �ƶ���ֵ�������ת������������Ȩ
        // ������other - Դ Ctx ����
        // ���أ���ǰ���������
        Ctx& operator=(Ctx&& other) noexcept {
            if (this != &other) {
                if (_My_ctx.id != 0) {
                    nng_ctx_close(_My_ctx);
                }
                _My_ctx = other._My_ctx;
                other._My_ctx = NNG_CTX_INITIALIZER;
            }
            return *this;
        }

        // �����������ͷ���������Դ
        ~Ctx() noexcept {
            if (_My_ctx.id != 0) {
                nng_ctx_close(_My_ctx);
            }
        }

        // ��ȡ������ ID
        // ���أ������ĵ� ID
        int id() const noexcept {
            return nng_ctx_id(_My_ctx);
        }

        // �첽������Ϣ
        // ������aio - �첽 I/O ����
        void recv(nng_aio* aio) const noexcept {
            nng_ctx_recv(_My_ctx, aio);
        }

        // ͬ��������Ϣ
        // ������flags - ���ձ�־��Ĭ��Ϊ 0
        // ���أ����յ��� Msg ����
        // �쳣��������ʧ�ܣ��׳� Exception
        Msg recv_msg(int flags = 0) const noexcept(false) {
            nng_msg* m;
            int rv = nng_ctx_recvmsg(_My_ctx, &m, flags);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_ctx_recvmsg");
            }
            return Msg(m);
        }

        // �첽������Ϣ
        // ������aio - �첽 I/O ����
        void send(nng_aio* aio) const noexcept {
            nng_ctx_send(_My_ctx, aio);
        }

        // �첽������Ϣ��ʹ�� Aio ����
        // ������aio - Aio ����
        void send(const Aio& aio) const noexcept {
            nng_ctx_send(_My_ctx, aio);
        }

        // ͬ��������Ϣ
        // ������msg - Ҫ���͵� nng_msg ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int send(nng_msg* msg) const noexcept {
            return nng_ctx_sendmsg(_My_ctx, msg, 0);
        }

        // ͬ��������������Ϣ
        // ������msg - Ҫ���͵� nng_msg ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int send_nonblock(nng_msg* msg) const noexcept {
            return nng_ctx_sendmsg(_My_ctx, msg, NNG_FLAG_NONBLOCK);
        }

        // ͬ��������Ϣ��ͨ���ƶ� Msg ����
        // ������msg - Ҫ���͵� Msg ����
        // ���أ����������0 ��ʾ�ɹ�
        // ע�⣺�����ͳɹ���msg ����Դ�ᱻ�ͷ�
        int send(Msg&& msg) const noexcept {
            int rv = nng_ctx_sendmsg(_My_ctx, msg, 0);
            if (rv == NNG_OK) {
                msg.release();
            }
            return rv;
        }

        // ��ȡ������������ѡ��
        // ������name - ѡ�����ƣ�value - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_bool(std::string_view name, bool* value) const noexcept {
            return nng_ctx_get_bool(_My_ctx, name.data(), value);
        }

        // ��ȡ����������ѡ��
        // ������name - ѡ�����ƣ�value - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_int(std::string_view name, int* value) const noexcept {
            return nng_ctx_get_int(_My_ctx, name.data(), value);
        }

        // ��ȡ��С��������ѡ��
        // ������name - ѡ�����ƣ�value - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_size(std::string_view name, size_t* value) const noexcept {
            return nng_ctx_get_size(_My_ctx, name.data(), value);
        }

        // ��ȡʱ����������ѡ��
        // ������name - ѡ�����ƣ�value - �洢ѡ��ֵ��ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int get_ms(std::string_view name, nng_duration* value) const noexcept {
            return nng_ctx_get_ms(_My_ctx, name.data(), value);
        }

        // ����������ѡ��
        // ������name - ѡ�����ƣ�data - ѡ�����ݣ�size - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int set(std::string_view name, const void* data, size_t size) const noexcept {
            return nng_ctx_set(_My_ctx, name.data(), data, size);
        }

        // ���ò�����������ѡ��
        // ������name - ѡ�����ƣ�value - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_bool(std::string_view name, bool value) const noexcept {
            return nng_ctx_set_bool(_My_ctx, name.data(), value);
        }

        // ��������������ѡ��
        // ������name - ѡ�����ƣ�value - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_int(std::string_view name, int value) const noexcept {
            return nng_ctx_set_int(_My_ctx, name.data(), value);
        }

        // ���ô�С��������ѡ��
        // ������name - ѡ�����ƣ�value - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_size(std::string_view name, size_t value) const noexcept {
            return nng_ctx_set_size(_My_ctx, name.data(), value);
        }

        // ����ʱ����������ѡ��
        // ������name - ѡ�����ƣ�value - ѡ��ֵ
        // ���أ����������0 ��ʾ�ɹ�
        int set_ms(std::string_view name, nng_duration value) const noexcept {
            return nng_ctx_set_ms(_My_ctx, name.data(), value);
        }

        // SUB0 Э�飺����ָ������
        // ������buf - �������ݣ�sz - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int subscribe(const void* buf, size_t sz) const noexcept {
            return nng_sub0_ctx_subscribe(_My_ctx, buf, sz);
        }

        // SUB0 Э�飺ȡ������ָ������
        // ������buf - �������ݣ�sz - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int unsubscribe(const void* buf, size_t sz) const noexcept {
            return nng_sub0_ctx_unsubscribe(_My_ctx, buf, sz);
        }

    private:
        nng_ctx _My_ctx = NNG_CTX_INITIALIZER;
    };
}