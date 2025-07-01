#pragma once

#include "nngException.h"
#include "nngHooker.h"

namespace nng
{
    // Dialer 类：NNG 拨号器（nng_dialer）的 C++ RAII 包装类
    // 用途：提供对 NNG 拨号器的便捷管理，支持拨号器的创建、启动、关闭、选项配置以及 TLS 和 URL 设置/获取
    // 特性：
    // - 使用 RAII 管理 nng_dialer 资源，确保自动释放
    // - 支持移动构造和移动赋值，禁用拷贝以保证资源独占
    // - 提供链式调用接口以设置选项
    // - 异常安全：拨号器创建失败时抛出 Exception
    class Dialer
        : public Hooker<Dialer> {
    public:
        // 构造函数：为指定套接字和地址创建拨号器
        // 参数：socket - NNG 套接字，addr - 连接地址
        // 异常：若创建失败，抛出 Exception
        Dialer(nng_socket socket, std::string_view addr) noexcept(false) {
            int rv = nng_dialer_create(&_My_dialer, socket, Hooker<Dialer>::_Pre_address(addr).c_str());
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_dialer_create");
            }
        }

        // 析构函数：关闭并释放拨号器资源
        virtual ~Dialer() noexcept {
            close();
        }

        // 移动构造函数：转移拨号器所有权
        // 参数：other - 源 Dialer 对象
        Dialer(Dialer&& other) noexcept : _My_dialer(other._My_dialer) {
            other._My_dialer.id = 0;
        }

        // 移动赋值运算符：转移拨号器所有权
        // 参数：other - 源 Dialer 对象
        // 返回：当前对象的引用
        Dialer& operator=(Dialer&& other) noexcept {
            if (this != &other) {
                close();
                _My_dialer = other._My_dialer;
                other._My_dialer.id = 0;
            }
            return *this;
        }

        // 禁用拷贝构造函数
        Dialer(const Dialer&) = delete;

        // 禁用拷贝赋值运算符
        Dialer& operator=(const Dialer&) = delete;

        // 启动拨号器
        // 参数：flags - 启动标志，默认为 0
        // 返回：操作结果，0 表示成功
        int start(int flags = 0) noexcept {
            _Pre_start(*this);
            return nng_dialer_start(_My_dialer, flags);
        }

        // 关闭拨号器
        void close() noexcept {
            if (valid()) {
                nng_dialer_close(_My_dialer);
                _My_dialer.id = 0;
            }
        }

        // 检查拨号器是否有效
        // 返回：true 表示有效，false 表示无效
        bool valid() const noexcept {
            return _My_dialer.id != 0;
        }

        // 获取拨号器 ID
        // 返回：拨号器的 ID
        int id() const noexcept {
            return nng_dialer_id(_My_dialer);
        }

        // 设置布尔型拨号器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Dialer& set_bool(std::string_view opt, bool val) noexcept {
            nng_dialer_set_bool(_My_dialer, opt.data(), val);
            return *this;
        }

        // 设置整型拨号器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Dialer& set_int(std::string_view opt, int val) noexcept {
            nng_dialer_set_int(_My_dialer, opt.data(), val);
            return *this;
        }

        // 设置大小型拨号器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Dialer& set_size(std::string_view opt, size_t val) noexcept {
            nng_dialer_set_size(_My_dialer, opt.data(), val);
            return *this;
        }

        // 设置 64 位无符号整型拨号器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Dialer& set_uint64(std::string_view opt, uint64_t val) noexcept {
            nng_dialer_set_uint64(_My_dialer, opt.data(), val);
            return *this;
        }

        // 设置字符串型拨号器选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Dialer& set_string(std::string_view opt, std::string_view val) noexcept {
            nng_dialer_set_string(_My_dialer, opt.data(), val.data());
            return *this;
        }

        // 设置时间型拨号器选项
        // 参数：opt - 选项名称，val - 选项值（毫秒）
        // 返回：当前对象的引用（支持链式调用）
        Dialer& set_ms(std::string_view opt, nng_duration val) noexcept {
            nng_dialer_set_ms(_My_dialer, opt.data(), val);
            return *this;
        }

        // 设置地址型拨号器选项
        // 参数：opt - 选项名称，val - 地址值
        // 返回：当前对象的引用（支持链式调用）
        Dialer& set_addr(std::string_view opt, const nng_sockaddr& val) noexcept {
            nng_dialer_set_addr(_My_dialer, opt.data(), &val);
            return *this;
        }

        // 获取布尔型拨号器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_bool(std::string_view opt, bool& val) const noexcept {
            return nng_dialer_get_bool(_My_dialer, opt.data(), &val);
        }

        // 获取整型拨号器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_int(std::string_view opt, int& val) const noexcept {
            return nng_dialer_get_int(_My_dialer, opt.data(), &val);
        }

        // 获取大小型拨号器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_size(std::string_view opt, size_t& val) const noexcept {
            return nng_dialer_get_size(_My_dialer, opt.data(), &val);
        }

        // 获取 64 位无符号整型拨号器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_uint64(std::string_view opt, uint64_t& val) const noexcept {
            return nng_dialer_get_uint64(_My_dialer, opt.data(), &val);
        }

        // 获取字符串型拨号器选项
        // 参数：opt - 选项名称，val - 存储选项值的字符串
        // 返回：操作结果，0 表示成功
        int get_string(std::string_view opt, std::string& val) const noexcept {
            char* out = nullptr;
            int rv = nng_dialer_get_string(_My_dialer, opt.data(), &out);
            if (rv == 0 && out != nullptr) {
                val = out;
                nng_strfree(out);
            }
            return rv;
        }

        // 获取时间型拨号器选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_ms(std::string_view opt, nng_duration& val) const noexcept {
            return nng_dialer_get_ms(_My_dialer, opt.data(), &val);
        }

        // 获取地址型拨号器选项
        // 参数：opt - 选项名称，val - 存储地址值的指针
        // 返回：操作结果，0 表示成功
        int get_addr(std::string_view opt, nng_sockaddr& val) const noexcept {
            return nng_dialer_get_addr(_My_dialer, opt.data(), &val);
        }

        // 获取拨号器的 URL
        // 参数：url - 存储 URL 的指针
        // 返回：操作结果，0 表示成功
        int get_url(const nng_url** url) const noexcept {
            return nng_dialer_get_url(_My_dialer, url);
        }

        // 转换为 nng_dialer
        // 返回：当前管理的 nng_dialer
        operator nng_dialer() const noexcept {
            return _My_dialer;
        }

    private:
        nng_dialer _My_dialer = NNG_DIALER_INITIALIZER;
    };
}