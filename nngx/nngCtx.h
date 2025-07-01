#pragma once

#include <string_view>

#include "nngException.h"
#include "nngSocket.h"
#include "nngMsg.h"
#include "nngAio.h"

namespace nng
{
    // Ctx 类：NNG 上下文（nng_ctx）的 C++ RAII 包装类
    // 用途：提供对 NNG 上下文的便捷管理，支持消息的异步和同步发送/接收、选项配置以及 SUB0 协议的订阅操作
    // 特性：
    // - 使用 RAII 管理 nng_ctx 资源，确保自动释放
    // - 支持移动构造和移动赋值，禁用拷贝以保证资源独占
    // - 提供异步和同步的消息操作、上下文选项配置
    // - 异常安全：上下文创建或接收消息失败时抛出 Exception
    class Ctx {
    public:
        // 构造函数：为指定套接字创建上下文
        // 参数：socket - NNG 套接字
        // 异常：若创建失败，抛出 Exception
        explicit Ctx(nng_socket socket) noexcept(false) {
            int rv = nng_ctx_open(&_My_ctx, socket);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_ctx_open");
            }
        }

        // 禁用拷贝构造函数
        Ctx(const Ctx&) = delete;

        // 禁用拷贝赋值运算符
        Ctx& operator=(const Ctx&) = delete;

        // 移动构造函数：转移上下文所有权
        // 参数：other - 源 Ctx 对象
        Ctx(Ctx&& other) noexcept : _My_ctx(other._My_ctx) {
            other._My_ctx = NNG_CTX_INITIALIZER;
        }

        // 移动赋值运算符：转移上下文所有权
        // 参数：other - 源 Ctx 对象
        // 返回：当前对象的引用
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

        // 析构函数：释放上下文资源
        ~Ctx() noexcept {
            if (_My_ctx.id != 0) {
                nng_ctx_close(_My_ctx);
            }
        }

        // 获取上下文 ID
        // 返回：上下文的 ID
        int id() const noexcept {
            return nng_ctx_id(_My_ctx);
        }

        // 异步接收消息
        // 参数：aio - 异步 I/O 对象
        void recv(nng_aio* aio) const noexcept {
            nng_ctx_recv(_My_ctx, aio);
        }

        // 同步接收消息
        // 参数：flags - 接收标志，默认为 0
        // 返回：接收到的 Msg 对象
        // 异常：若接收失败，抛出 Exception
        Msg recv_msg(int flags = 0) const noexcept(false) {
            nng_msg* m;
            int rv = nng_ctx_recvmsg(_My_ctx, &m, flags);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_ctx_recvmsg");
            }
            return Msg(m);
        }

        // 异步发送消息
        // 参数：aio - 异步 I/O 对象
        void send(nng_aio* aio) const noexcept {
            nng_ctx_send(_My_ctx, aio);
        }

        // 异步发送消息（使用 Aio 对象）
        // 参数：aio - Aio 对象
        void send(const Aio& aio) const noexcept {
            nng_ctx_send(_My_ctx, aio);
        }

        // 同步发送消息
        // 参数：msg - 要发送的 nng_msg 指针
        // 返回：操作结果，0 表示成功
        int send(nng_msg* msg) const noexcept {
            return nng_ctx_sendmsg(_My_ctx, msg, 0);
        }

        // 同步非阻塞发送消息
        // 参数：msg - 要发送的 nng_msg 指针
        // 返回：操作结果，0 表示成功
        int send_nonblock(nng_msg* msg) const noexcept {
            return nng_ctx_sendmsg(_My_ctx, msg, NNG_FLAG_NONBLOCK);
        }

        // 同步发送消息（通过移动 Msg 对象）
        // 参数：msg - 要发送的 Msg 对象
        // 返回：操作结果，0 表示成功
        // 注意：若发送成功，msg 的资源会被释放
        int send(Msg&& msg) const noexcept {
            int rv = nng_ctx_sendmsg(_My_ctx, msg, 0);
            if (rv == NNG_OK) {
                msg.release();
            }
            return rv;
        }

        // 获取布尔型上下文选项
        // 参数：name - 选项名称，value - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_bool(std::string_view name, bool* value) const noexcept {
            return nng_ctx_get_bool(_My_ctx, name.data(), value);
        }

        // 获取整型上下文选项
        // 参数：name - 选项名称，value - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_int(std::string_view name, int* value) const noexcept {
            return nng_ctx_get_int(_My_ctx, name.data(), value);
        }

        // 获取大小型上下文选项
        // 参数：name - 选项名称，value - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_size(std::string_view name, size_t* value) const noexcept {
            return nng_ctx_get_size(_My_ctx, name.data(), value);
        }

        // 获取时间型上下文选项
        // 参数：name - 选项名称，value - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_ms(std::string_view name, nng_duration* value) const noexcept {
            return nng_ctx_get_ms(_My_ctx, name.data(), value);
        }

        // 设置上下文选项
        // 参数：name - 选项名称，data - 选项数据，size - 数据大小
        // 返回：操作结果，0 表示成功
        int set(std::string_view name, const void* data, size_t size) const noexcept {
            return nng_ctx_set(_My_ctx, name.data(), data, size);
        }

        // 设置布尔型上下文选项
        // 参数：name - 选项名称，value - 选项值
        // 返回：操作结果，0 表示成功
        int set_bool(std::string_view name, bool value) const noexcept {
            return nng_ctx_set_bool(_My_ctx, name.data(), value);
        }

        // 设置整型上下文选项
        // 参数：name - 选项名称，value - 选项值
        // 返回：操作结果，0 表示成功
        int set_int(std::string_view name, int value) const noexcept {
            return nng_ctx_set_int(_My_ctx, name.data(), value);
        }

        // 设置大小型上下文选项
        // 参数：name - 选项名称，value - 选项值
        // 返回：操作结果，0 表示成功
        int set_size(std::string_view name, size_t value) const noexcept {
            return nng_ctx_set_size(_My_ctx, name.data(), value);
        }

        // 设置时间型上下文选项
        // 参数：name - 选项名称，value - 选项值
        // 返回：操作结果，0 表示成功
        int set_ms(std::string_view name, nng_duration value) const noexcept {
            return nng_ctx_set_ms(_My_ctx, name.data(), value);
        }

        // SUB0 协议：订阅指定主题
        // 参数：buf - 主题数据，sz - 数据大小
        // 返回：操作结果，0 表示成功
        int subscribe(const void* buf, size_t sz) const noexcept {
            return nng_sub0_ctx_subscribe(_My_ctx, buf, sz);
        }

        // SUB0 协议：取消订阅指定主题
        // 参数：buf - 主题数据，sz - 数据大小
        // 返回：操作结果，0 表示成功
        int unsubscribe(const void* buf, size_t sz) const noexcept {
            return nng_sub0_ctx_unsubscribe(_My_ctx, buf, sz);
        }

    private:
        nng_ctx _My_ctx = NNG_CTX_INITIALIZER;
    };
}