#pragma once

#include <nng/nng.h>
#include <cassert>

namespace nng {
    // SocketCore 类：NNG 套接字（nng_socket）的极简 RAII 包装类
    // 用途：为 nng_socket 提供最小化的 C++ RAII 管理，便于资源安全释放和基本操作
    // 特性：
    // - 使用 RAII 管理 nng_socket 资源，确保自动释放
    // - 支持移动构造和移动赋值，禁用拷贝以保证资源独占
    // - 提供基础的套接字创建、关闭、选项设置与获取等接口
    // - 仅封装最核心的 socket 操作，适合底层扩展或自定义高级封装
    class SocketCore {
    public:
        using _Socket_creator_t = int (*)(nng_socket*); // 套接字创建函数类型

        // 构造函数：创建未初始化的套接字对象
        SocketCore() noexcept = default;

        // 构造函数：使用现有 nng_socket 初始化
        explicit SocketCore(nng_socket socket) noexcept : _My_socket(socket) {}

        // 构造函数：通过创建函数创建套接字
        // 参数：creator - nng_xxx_open 之类的创建函数
        // 异常：若创建失败，抛出错误码
        explicit SocketCore(_Socket_creator_t creator) noexcept(false) {
            assert(creator);
            int rv = creator(&_My_socket);
            if (rv != 0) {
                throw rv; // 极简风格，直接抛出错误码
            }
        }

        // 析构函数：自动关闭套接字
        ~SocketCore() noexcept {
            close();
        }

        // 移动构造函数：转移 socket 所有权
        SocketCore(SocketCore&& other) noexcept : _My_socket(other._My_socket) {
            other._My_socket.id = 0;
        }

        // 移动赋值运算符：转移 socket 所有权
        SocketCore& operator=(SocketCore&& other) noexcept {
            if (this != &other) {
                close();
                _My_socket = other._My_socket;
                other._My_socket.id = 0;
            }
            return *this;
        }

        // 禁用拷贝构造和拷贝赋值，保证资源独占
        SocketCore(const SocketCore&) = delete;
        SocketCore& operator=(const SocketCore&) = delete;

        // 创建套接字（通过创建函数）
        // 参数：creator - nng_xxx_open 之类的创建函数
        // 返回：0 表示成功
        int create(_Socket_creator_t creator) noexcept {
            assert(creator);
            assert(_My_socket.id == 0);
            return creator(&_My_socket);
        }

        // 关闭套接字
        void close() noexcept {
            if (valid()) {
                nng_socket_close(_My_socket);
                _My_socket.id = 0;
            }
        }

        // 检查套接字是否有效
        bool valid() const noexcept {
            return _My_socket.id != 0;
        }

        // 获取底层 nng_socket
        nng_socket get() const noexcept {
            return _My_socket;
        }

        // 隐式转换为 nng_socket
        operator nng_socket() const noexcept {
            return _My_socket;
        }

        // 获取套接字 id
        int id() const noexcept {
            return nng_socket_id(_My_socket);
        }

        // 设置套接字选项
        int set_bool(const char* opt, bool val) noexcept {
            return nng_socket_set_bool(_My_socket, opt, val);
        }
        int set_int(const char* opt, int val) noexcept {
            return nng_socket_set_int(_My_socket, opt, val);
        }
        int set_size(const char* opt, size_t val) noexcept {
            return nng_socket_set_size(_My_socket, opt, val);
        }
        int set_ms(const char* opt, nng_duration val) noexcept {
            return nng_socket_set_ms(_My_socket, opt, val);
        }

        // 获取套接字选项
        int get_bool(const char* opt, bool& val) const noexcept {
            return nng_socket_get_bool(_My_socket, opt, &val);
        }
        int get_int(const char* opt, int& val) const noexcept {
            return nng_socket_get_int(_My_socket, opt, &val);
        }
        int get_size(const char* opt, size_t& val) const noexcept {
            return nng_socket_get_size(_My_socket, opt, &val);
        }
        int get_ms(const char* opt, nng_duration& val) const noexcept {
            return nng_socket_get_ms(_My_socket, opt, &val);
        }
        int get_proto_id(uint16_t& id) const noexcept {
            return nng_socket_proto_id(_My_socket, &id);
        }
        int get_peer_id(uint16_t& id) const noexcept {
            return nng_socket_peer_id(_My_socket, &id);
        }
        int get_proto_name(const char** name) const noexcept {
            return nng_socket_proto_name(_My_socket, name);
        }
        int get_peer_name(const char** name) const noexcept {
            return nng_socket_peer_name(_My_socket, name);
        }
        int get_raw(bool& raw) const noexcept {
            return nng_socket_raw(_My_socket, &raw);
        }

        // 设置管道通知回调
        int pipe_notify(nng_pipe_ev ev, nng_pipe_cb cb, void* arg) noexcept {
            return nng_pipe_notify(_My_socket, ev, cb, arg);
        }

    protected:
        nng_socket _My_socket = NNG_SOCKET_INITIALIZER; // 底层 nng_socket 句柄
    };
} 