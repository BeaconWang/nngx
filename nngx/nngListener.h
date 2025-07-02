#pragma once

#include "nngException.h"
#include "nngHooker.h"

namespace nng
{
    // Listener 类：NNG 监听器（nng_listener）的 C++ RAII 包装类
    // 用途：提供对 NNG 监听器的便捷管理，支持监听器的创建、启动、关闭和选项配置
    // 特性：
    // - 使用 RAII 管理 nng_listener 资源，确保自动释放
    // - 支持移动构造和移动赋值，禁用拷贝以保证资源独占
    // - 提供选项配置接口，支持多种数据类型的设置和获取
    // - 异常安全：监听器创建失败时抛出 Exception
    class Listener
        : public Hooker<Listener> {
    public:
        // 默认构造函数：创建未初始化的监听器
        Listener() noexcept = default;

        // 构造函数：为指定套接字和地址创建监听器
        // 参数：sock - NNG 套接字，addr - 监听地址
        // 异常：若创建失败，抛出 Exception
        Listener(nng_socket sock, std::string_view addr) noexcept(false) {
            int rv = nng_listener_create(&_My_listener, sock, Hooker<Listener>::_Pre_address(addr).c_str());
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_listener_create");
            }
        }

        // 析构函数：关闭并释放监听器资源
        virtual ~Listener() noexcept {
            close();
        }

        // 移动构造函数：转移监听器所有权
        // 参数：other - 源 Listener 对象
        Listener(Listener&& other) noexcept : _My_listener(other._My_listener) {
            other._My_listener.id = 0;
        }

        // 移动赋值运算符：转移监听器所有权
        // 参数：other - 源 Listener 对象
        // 返回：当前对象的引用
        Listener& operator=(Listener&& other) noexcept {
            if (this != &other) {
                close();
                _My_listener = other._My_listener;
                other._My_listener.id = 0;
            }
            return *this;
        }

        // 禁用拷贝构造函数
        Listener(const Listener&) = delete;

        // 禁用拷贝赋值运算符
        Listener& operator=(const Listener&) = delete;

        // 启动监听器
        // 参数：flags - 启动标志，默认为 0
        // 返回：操作结果，0 表示成功
        int start(int flags = 0) noexcept {
            _Pre_start(*this);
            return nng_listener_start(_My_listener, flags);
        }

        // 关闭监听器
        void close() noexcept {
            if (valid()) {
                nng_listener_close(_My_listener);
                _My_listener.id = 0;
            }
        }

        // 获取监听器 ID
        // 返回：监听器的 ID
        int id() const noexcept {
            return nng_listener_id(_My_listener);
        }

        // 检查监听器是否有效
        // 返回：true 表示有效，false 表示无效
        bool valid() const noexcept {
            return _My_listener.id != 0;
        }

        // 设置指针型监听器选项
        // 参数：opt - 选项名称，val - 选项值（指针）
        // 返回：操作结果，0 表示成功
        int set_ptr(std::string_view opt, void* val) noexcept {
            return nng_listener_set_ptr(_My_listener, opt.data(), val);
        }

        // 获取指针型监听器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_ptr(std::string_view opt, void** val) const noexcept {
            return nng_listener_get_ptr(_My_listener, opt.data(), val);
        }

        // 设置布尔型监听器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：操作结果，0 表示成功
        int set_bool(std::string_view opt, bool val) noexcept {
            return nng_listener_set_bool(_My_listener, opt.data(), val);
        }

        // 设置整型监听器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：操作结果，0 表示成功
        int set_int(std::string_view opt, int val) noexcept {
            return nng_listener_set_int(_My_listener, opt.data(), val);
        }

        // 设置大小型监听器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：操作结果，0 表示成功
        int set_size(std::string_view opt, size_t val) noexcept {
            return nng_listener_set_size(_My_listener, opt.data(), val);
        }

        // 设置时间型监听器选项
        // 参数：opt - 选项名称，val - 选项值（毫秒）
        // 返回：操作结果，0 表示成功
        int set_ms(std::string_view opt, nng_duration val) noexcept {
            return nng_listener_set_ms(_My_listener, opt.data(), val);
        }

        // 设置 64 位无符号整型监听器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：操作结果，0 表示成功
        int set_uint64(std::string_view opt, uint64_t val) noexcept {
            return nng_listener_set_uint64(_My_listener, opt.data(), val);
        }

        // 设置字符串型监听器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：操作结果，0 表示成功
        int set_string(std::string_view opt, std::string_view val) noexcept {
            return nng_listener_set_string(_My_listener, opt.data(), val.data());
        }

        // 获取布尔型监听器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_bool(std::string_view opt, bool& val) const noexcept {
            return nng_listener_get_bool(_My_listener, opt.data(), &val);
        }

        // 获取整型监听器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_int(std::string_view opt, int& val) const noexcept {
            return nng_listener_get_int(_My_listener, opt.data(), &val);
        }

        // 获取大小型监听器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_size(std::string_view opt, size_t& val) const noexcept {
            return nng_listener_get_size(_My_listener, opt.data(), &val);
        }

        // 获取时间型监听器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_ms(std::string_view opt, nng_duration& val) const noexcept {
            return nng_listener_get_ms(_My_listener, opt.data(), &val);
        }

        // 获取 64 位无符号整型监听器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_uint64(std::string_view opt, uint64_t& val) const noexcept {
            return nng_listener_get_uint64(_My_listener, opt.data(), &val);
        }

        // 获取字符串型监听器选项
        // 参数：opt - 选项名称，val - 存储选项值的字符串
        // 返回：操作结果，0 表示成功
        int get_string(std::string_view opt, std::string& val) const noexcept {
            char* out = nullptr;
            int rv = nng_listener_get_string(_My_listener, opt.data(), &out);
            if (rv == 0 && out) {
                val = out;
                nng_strfree(out);
            }
            return rv;
        }

        // 转换为 nng_listener
        // 返回：当前管理的 nng_listener
        operator nng_listener() const noexcept {
            return _My_listener;
        }

    private:
        nng_listener _My_listener = NNG_LISTENER_INITIALIZER;
    };
}