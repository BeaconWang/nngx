#pragma once

#include <cassert>
#include <stdexcept>

#include <functional>
#include <mutex>

#define NNG_STATIC_LIB

#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>
#include <nng/protocol/pipeline0/push.h>
#include <nng/protocol/pipeline0/pull.h>
#include <nng/protocol/survey0/survey.h>
#include <nng/protocol/survey0/respond.h>
#include <nng/protocol/pair0/pair.h>
#include <nng/protocol/bus0/bus.h>

namespace nng
{
    enum { NNG_OK = 0 };
    using nng_err = int;
    // Exception �ࣺNNG �����루nng_err���� C++ �쳣��װ��
    // ��;����װ NNG ������ʹ�����Ϣ���ṩ�쳣������ƣ��̳��� std::runtime_error
    // ���ԣ�
    // - �ṩ NNG ������Ĵ洢�Ͳ�ѯ
    // - �쳣��ȫ�����캯�������׳� std::bad_alloc
    // - ������ƣ��� NNG �������޷켯��
    class Exception : public std::runtime_error {
    public:
        // ���캯����ʹ�� NNG ���������Ϣ��ʼ���쳣
        // ������err - NNG �����룬msg - ������Ϣ
        // �쳣�������׳� std::bad_alloc���� std::runtime_error ������
        Exception(nng_err err, std::string_view msg)
            noexcept(false) : std::runtime_error(msg.data()), _My_err(err) {}
        // ��ȡ NNG ������
        // ���أ��洢�� NNG ������
        nng_err get_error() const noexcept { return _My_err; }

    private:
        nng_err _My_err;
    };
}