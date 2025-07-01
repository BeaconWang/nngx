#pragma once

#include "nngException.h"
#include "nngMsg.h"

namespace nng
{
    // Aio 类：NNG 异步 I/O（nng_aio）的 C++ RAII 包装类
    // 用途：提供对 NNG 异步 I/O 操作的便捷管理，支持设置消息、输入/输出参数、超时、取消和等待等操作
    // 特性：
    // - 使用 RAII 管理 nng_aio 资源，确保自动释放
    // - 支持移动构造和移动赋值，禁用拷贝以保证资源独占
    // - 提供异步操作的配置和查询功能
    // - 异常安全：分配失败时抛出 Exception
    class Aio {
    public:
        using _Callback_t = void (*)(void*);

        // 构造函数：初始化异步 I/O 对象
        // 参数：callback - 回调函数指针，callback_context - 回调上下文指针
        // 异常：若分配失败，抛出 Exception
        Aio(_Callback_t callback = nullptr, void* callback_context = nullptr) noexcept(false) {
            int rv = nng_aio_alloc(&_My_aio, callback, callback_context);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_aio_alloc");
            }
        }

        // 析构函数：释放异步 I/O 资源
        virtual ~Aio() noexcept {
            if (_My_aio) {
                nng_aio_free(_My_aio);
            }
        }

        // 禁用拷贝构造函数
        Aio(const Aio&) = delete;

        // 禁用拷贝赋值运算符
        Aio& operator=(const Aio&) = delete;

        // 移动构造函数：转移异步 I/O 所有权
        // 参数：other - 源 Aio 对象
        Aio(Aio&& other) noexcept : _My_aio(other._My_aio) {
            other._My_aio = nullptr;
        }

        // 移动赋值运算符：转移异步 I/O 所有权
        // 参数：other - 源 Aio 对象
        // 返回：当前对象的引用
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

        // 设置异步 I/O 的消息
        // 参数：msg - 要设置的 nng_msg 指针
        void set_msg(nng_msg* msg) noexcept {
            nng_aio_set_msg(_My_aio, msg);
        }

        // 设置异步 I/O 的消息（通过移动 Msg 对象）
        // 参数：msg - 要设置的 Msg 对象
        void set_msg(Msg&& msg) noexcept {
            nng_aio_set_msg(_My_aio, msg.release());
        }

        // 获取异步 I/O 的消息
        // 返回：包含当前消息的 Msg 对象
        Msg get_msg() const noexcept {
            return Msg(nng_aio_get_msg(_My_aio));
        }

        // 释放异步 I/O 的消息
        // 返回：包含释放消息的 Msg 对象
        Msg release_msg() const noexcept {
            auto m = nng_aio_get_msg(_My_aio);
            nng_aio_set_msg(_My_aio, nullptr);
            return Msg(m);
        }

        // 设置异步 I/O 的超时时间
        // 参数：timeout - 超时时间（毫秒）
        void set_timeout(nng_duration timeout) noexcept {
            nng_aio_set_timeout(_My_aio, timeout);
        }

        // 设置异步 I/O 的绝对过期时间
        // 参数：expire - 绝对过期时间
        void set_expire(nng_time expire) noexcept {
            nng_aio_set_expire(_My_aio, expire);
        }

        // 设置异步 I/O 的 I/O 向量
        // 参数：cnt - I/O 向量数量，iov - I/O 向量数组
        // 返回：操作结果，0 表示成功
        int set_iov(unsigned cnt, const nng_iov* iov) noexcept {
            return nng_aio_set_iov(_My_aio, cnt, iov);
        }

        // 设置异步 I/O 的输入参数
        // 参数：index - 输入参数索引，data - 输入参数数据
        // 返回：操作结果，0 表示成功
        int set_input(unsigned index, void* data) noexcept {
            return nng_aio_set_input(_My_aio, index, data);
        }

        // 获取异步 I/O 的输入参数
        // 参数：index - 输入参数索引
        // 返回：输入参数数据指针
        void* get_input(unsigned index) const noexcept {
            return nng_aio_get_input(_My_aio, index);
        }

        // 设置异步 I/O 的输出参数
        // 参数：index - 输出参数索引，data - 输出参数数据
        // 返回：操作结果，0 表示成功
        int set_output(unsigned index, void* data) noexcept {
            return nng_aio_set_output(_My_aio, index, data);
        }

        // 获取异步 I/O 的输出参数
        // 参数：index - 输出参数索引
        // 返回：输出参数数据指针
        void* get_output(unsigned index) const noexcept {
            return nng_aio_get_output(_My_aio, index);
        }

        // 取消异步 I/O 操作
        void cancel() noexcept {
            nng_aio_cancel(_My_aio);
        }

        // 中止异步 I/O 操作
        // 参数：err - 中止操作的错误码
        void abort(nng_err err) noexcept {
            nng_aio_abort(_My_aio, err);
        }

        // 等待异步 I/O 操作完成
        void wait() noexcept {
            nng_aio_wait(_My_aio);
        }

        // 执行异步睡眠操作
        // 参数：ms - 睡眠时间（毫秒）
        void sleep(nng_duration ms) noexcept {
            nng_sleep_aio(ms, _My_aio);
        }

        // 检查异步 I/O 是否忙碌
        // 返回：true 表示忙碌，false 表示空闲
        bool is_busy() const noexcept {
            return nng_aio_busy(_My_aio);
        }

        // 获取异步 I/O 操作的结果
        // 返回：操作结果的错误码
        nng_err result() const noexcept {
            return nng_aio_result(_My_aio);
        }

        // 获取异步 I/O 的计数
        // 返回：操作的计数
        size_t count() const noexcept {
            return nng_aio_count(_My_aio);
        }

        // 转换为 nng_aio 指针
        // 返回：当前管理的 nng_aio 指针
        operator nng_aio* () const noexcept {
            return _My_aio;
        }

    private:
        nng_aio* _My_aio = nullptr;
    };
}