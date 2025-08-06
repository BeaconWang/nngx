#pragma once

#include <cassert>
#include <stdexcept>

#include <functional>
#include <mutex>
#include <memory>
#include <atomic>

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
    // Exception 类：NNG 错误码（nng_err）的 C++ 异常包装类
    // 用途：封装 NNG 错误码和错误消息，提供异常处理机制，继承自 std::runtime_error
    // 特性：
    // - 提供 NNG 错误码的存储和查询
    // - 异常安全：构造函数可能抛出 std::bad_alloc
    // - 轻量设计，与 NNG 错误码无缝集成
    class Exception : public std::runtime_error {
    public:
        // 构造函数：使用 NNG 错误码和消息初始化异常
        // 参数：err - NNG 错误码，msg - 错误消息
        // 异常：可能抛出 std::bad_alloc（由 std::runtime_error 引发）
        Exception(nng_err err, std::string_view msg)
            noexcept(false) : std::runtime_error(msg.data()), _My_err(err) {}
        // 获取 NNG 错误码
        // 返回：存储的 NNG 错误码
        nng_err get_error() const noexcept { return _My_err; }

    private:
        nng_err _My_err;
    };
}