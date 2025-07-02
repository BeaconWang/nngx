#pragma once

#include "nngException.h"
#include "nngMsg.h"

namespace nng
{
    // Socket 类：NNG 套接字（nng_socket）的 C++ RAII 包装类
    // 用途：提供对 NNG 套接字的便捷管理，支持套接字创建、关闭、消息发送/接收、选项配置及特定协议操作
    // 特性：
    // - 使用 RAII 管理 nng_socket 资源，确保自动释放
    // - 支持移动构造和移动赋值，禁用拷贝以保证资源独占
    // - 提供链式调用接口以设置选项
    // - 提供静态工厂方法以创建特定协议的套接字（如 req、rep、pub 等）
    // - 异常安全：套接字创建或部分发送/接收操作失败时抛出 Exception
    class Socket {
    public:
        using _Socket_creator_t = int (*)(nng_socket*);

    public:
        // 默认构造函数：创建未初始化的套接字
        explicit Socket() noexcept = default;

        // 构造函数：使用现有 nng_socket 初始化套接字
        // 参数：socket - NNG 套接字
        explicit Socket(nng_socket socket) noexcept : _My_socket(socket) {}

        // 构造函数：使用指定的创建函数初始化套接字
        // 参数：creator - 套接字创建函数
        // 异常：若创建失败，抛出 Exception
        explicit Socket(_Socket_creator_t creator) noexcept(false) {
            auto rv = creator(&_My_socket);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_xxx_open");
            }
        }

        // 析构函数：关闭并释放套接字资源
        virtual ~Socket() noexcept {
            close();
        }

        // 移动构造函数：转移套接字所有权
        // 参数：other - 源 Socket 对象
        Socket(Socket&& other) noexcept : _My_socket(other._My_socket) {
            other._My_socket.id = 0;
        }

        // 移动赋值运算符：转移套接字所有权
        // 参数：other - 源 Socket 对象
        // 返回：当前对象的引用
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                close();
                _My_socket = other._My_socket;
                other._My_socket.id = 0;
            }
            return *this;
        }

        // 禁用拷贝构造函数
        Socket(const Socket&) = delete;

        // 禁用拷贝赋值运算符
        Socket& operator=(const Socket&) = delete;

        // 创建套接字
        // 参数：creator - 套接字创建函数
        // 返回：操作结果，0 表示成功
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
        // 返回：true 表示有效，false 表示无效
        bool valid() const noexcept {
            return _My_socket.id != 0;
        }

        // 获取套接字 ID
        // 返回：套接字的 ID
        int id() const noexcept {
            return nng_socket_id(_My_socket);
        }

        // 静态工厂方法：创建 req 协议套接字
        // 返回：req 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket req() noexcept(false) { return _Create(nng_req0_open); }

        // 静态工厂方法：创建 rep 协议套接字
        // 返回：rep 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket rep() noexcept(false) { return _Create(nng_rep0_open); }

        // 静态工厂方法：创建 pub 协议套接字
        // 返回：pub 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket pub() noexcept(false) { return _Create(nng_pub0_open); }

        // 静态工厂方法：创建 sub 协议套接字
        // 返回：sub 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket sub() noexcept(false) { return _Create(nng_sub0_open); }

        // 静态工厂方法：创建 push 协议套接字
        // 返回：push 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket push() noexcept(false) { return _Create(nng_push0_open); }

        // 静态工厂方法：创建 pull 协议套接字
        // 返回：pull 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket pull() noexcept(false) { return _Create(nng_pull0_open); }

        // 静态工厂方法：创建 surveyor 协议套接字
        // 返回：surveyor 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket surveyor() noexcept(false) { return _Create(nng_surveyor0_open); }

        // 静态工厂方法：创建 respondent 协议套接字
        // 返回：respondent 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket respond() noexcept(false) { return _Create(nng_respondent0_open); }

        // 静态工厂方法：创建 bus 协议套接字
        // 返回：bus 协议的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket bus() noexcept(false) { return _Create(nng_bus0_open); }

        // 静态工厂方法：创建原始模式的 req 协议套接字
        // 返回：原始模式的 req 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket req_raw() noexcept(false) { return _Create(nng_req0_open_raw); }

        // 静态工厂方法：创建原始模式的 rep 协议套接字
        // 返回：原始模式的 rep 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket rep_raw() noexcept(false) { return _Create(nng_rep0_open_raw); }

        // 静态工厂方法：创建原始模式的 pub 协议套接字
        // 返回：原始模式的 pub 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket pub_raw() noexcept(false) { return _Create(nng_pub0_open_raw); }

        // 静态工厂方法：创建原始模式的 sub 协议套接字
        // 返回：原始模式的 sub 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket sub_raw() noexcept(false) { return _Create(nng_sub0_open_raw); }

        // 	static工厂方法：创建原始模式的 push 协议套接字
        // 	返回：原始模式的 push 协议 Socket 对象
        // 	异常：若创建失败，抛出 Exception
        static Socket push_raw() noexcept(false) { return _Create(nng_push0_open_raw); }

        // 静态工厂方法：创建原始模式的 pull 协议套接字
        // 返回：原始模式的 pull 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket pull_raw() noexcept(false) { return _Create(nng_pull0_open_raw); }

        // 静态工厂方法：创建原始模式的 surveyor 协议套接字
        // 返回：原始模式的 surveyor 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket surveyor_raw() noexcept(false) { return _Create(nng_surveyor0_open_raw); }

        // 静态工厂方法：创建原始模式的 respondent 协议套接字
        // 返回：原始模式的 respondent 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket respond_raw() noexcept(false) { return _Create(nng_respondent0_open_raw); }

        // 静态工厂方法：创建原始模式的 bus 协议套接字
        // 返回：原始模式的 bus 协议 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket bus_raw() noexcept(false) { return _Create(nng_bus0_open_raw); }

        // 设置布尔型套接字选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Socket& set_bool(std::string_view opt, bool val) noexcept {
            nng_socket_set_bool(_My_socket, opt.data(), val);
            return *this;
        }

        // 设置整型套接字选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Socket& set_int(std::string_view opt, int val) noexcept {
            nng_socket_set_int(_My_socket, opt.data(), val);
            return *this;
        }

        // 设置大小型套接字选项
        // 参数：opt - 选项名称，val - 选项值
        // 返回：当前对象的引用（支持链式调用）
        Socket& set_size(std::string_view opt, size_t val) noexcept {
            nng_socket_set_size(_My_socket, opt.data(), val);
            return *this;
        }

        // 设置时间型套接字选项
        // 参数：opt - 选项名称，val - 选项值（毫秒）
        // 返回：当前对象的引用（支持链式调用）
        Socket& set_ms(std::string_view opt, nng_duration val) noexcept {
            nng_socket_set_ms(_My_socket, opt.data(), val);
            return *this;
        }

        // 获取布尔型套接字选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_bool(std::string_view opt, bool& val) const noexcept {
            return nng_socket_get_bool(_My_socket, opt.data(), &val);
        }

        // 获取整型套接字选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_int(std::string_view opt, int& val) const noexcept {
            return nng_socket_get_int(_My_socket, opt.data(), &val);
        }

        // 获取大小型套接字选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_size(std::string_view opt, size_t& val) const noexcept {
            return nng_socket_get_size(_My_socket, opt.data(), &val);
        }

        // 获取时间型套接字选项
        // 参数：opt - 选项名称，val - 存储选项值的指针
        // 返回：操作结果，0 表示成功
        int get_ms(std::string_view opt, nng_duration& val) const noexcept {
            return nng_socket_get_ms(_My_socket, opt.data(), &val);
        }

        // 获取协议 ID
        // 参数：id - 存储协议 ID 的指针
        // 返回：操作结果，0 表示成功
        int get_proto_id(uint16_t& id) const noexcept {
            return nng_socket_proto_id(_My_socket, &id);
        }

        // 获取对端协议 ID
        // 参数：id - 存储对端协议 ID 的指针
        // 返回：操作结果，0 表示成功
        int get_peer_id(uint16_t& id) const noexcept {
            return nng_socket_peer_id(_My_socket, &id);
        }

        // 获取协议名称
        // 参数：name - 存储协议名称的指针
        // 返回：操作结果，0 表示成功
        int get_proto_name(const char** name) const noexcept {
            return nng_socket_proto_name(_My_socket, name);
        }

        // 获取对端协议名称
        // 参数：name - 存储对端协议名称的指针
        // 返回：操作结果，0 表示成功
        int get_peer_name(const char** name) const noexcept {
            return nng_socket_peer_name(_My_socket, name);
        }

        // 获取套接字是否为原始模式
        // 参数：raw - 存储原始模式标志的指针
        // 返回：操作结果，0 表示成功
        int get_raw(bool& raw) const noexcept {
            return nng_socket_raw(_My_socket, &raw);
        }

        // 格式化套接字地址
        // 参数：sa - 套接字地址，buf - 存储格式化结果的缓冲区，bufsz - 缓冲区大小
        // 返回：格式化后的地址字符串
        static const char* format_sockaddr(const nng_sockaddr& sa, char* buf, size_t bufsz) noexcept {
            return nng_str_sockaddr(&sa, buf, bufsz);
        }

        // SUB0 协议：订阅指定主题
        // 参数：sv - 主题字符串，默认为空
        // 返回：操作结果，0 表示成功
        int socket_subscribe(std::string_view sv = "") noexcept {
            return nng_sub0_socket_subscribe(_My_socket, sv.data(), sv.length());
        }

        // SUB0 协议：取消订阅指定主题
        // 参数：sv - 主题字符串，默认为空
        // 返回：操作结果，0 表示成功
        int socket_unsubscribe(std::string_view sv = "") noexcept {
            return nng_sub0_socket_unsubscribe(_My_socket, sv.data(), sv.length());
        }

        // 异步发送消息
        // 参数：aio - 异步 I/O 对象
        void send(nng_aio* aio) noexcept {
            nng_send_aio(_My_socket, aio);
        }

        // 异步接收消息
        // 参数：aio - 异步 I/O 对象
        void recv(nng_aio* aio) noexcept {
            nng_recv_aio(_My_socket, aio);
        }

        // 同步发送数据
        // 参数：data - 数据指针，data_size - 数据大小
        // 返回：操作结果，0 表示成功
        int send(const void* data, size_t data_size) noexcept {
            return nng_send(_My_socket, (void*)data, data_size, 0);
        }

        // 同步发送 I/O 向量数据
        // 参数：iov - I/O 向量
        // 返回：操作结果，0 表示成功
        int send(const nng_iov& iov) noexcept {
            return nng_send(_My_socket, iov.iov_buf, iov.iov_len, 0);
        }

        // 发送消息并接收返回结果
        // 参数：code - 消息代码，msg - 消息对象
        // 返回：消息结果
        // 异常：若消息重分配或发送失败，抛出 Exception
        Msg::_Ty_msg_result send(Msg::_Ty_msg_code code, Msg& msg) noexcept(false) {
            int rv;
            if (!msg) {
                rv = msg.realloc(0);
                if (rv != NNG_OK) {
                    throw Exception(rv, "realloc");
                }
            }

            Msg::_Append_msg_code(msg, code);

            rv = nng_sendmsg(_My_socket, msg, 0);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_sendmsg");
            }

            msg = recv();

            return Msg::_Chop_msg_result(msg);
        }

        // 发送消息并接收返回消息
        // 参数：msg - 消息对象
        // 返回：操作结果，0 表示成功
        int send(Msg& msg) noexcept {
            int rv;
            if (!msg) {
                rv = msg.realloc(0);
                if (rv != NNG_OK) {
                    return rv;
                }
            }

            rv = send(std::move(msg));
            if (rv == NNG_OK) {
                msg.release();
            }

            return recv(msg);
        }

        // 发送消息（不接收返回）
        // 参数：msg - 消息对象
        // 返回：操作结果，0 表示成功
        // 注意：若发送成功，msg 的资源会被释放
        int send(Msg&& msg) noexcept {
            int rv = nng_sendmsg(_My_socket, msg, 0);
            if (rv == NNG_OK) {
                msg.release();
            }
            return rv;
        }

        // 发送带消息代码的消息（不接收返回）
        // 参数：code - 消息代码，msg - 消息对象（默认为空）
        // 返回：操作结果，0 表示成功
        // 注意：若发送成功，msg 的资源会被释放
        int send(Msg::_Ty_msg_code code, Msg&& msg = Msg{}) noexcept {
            int rv;
            if (!msg) {
                rv = msg.realloc(0);
                if (rv != NNG_OK) {
                    return rv;
                }
            }

            Msg::_Append_msg_code(msg, code);

            rv = send(std::move(msg));
            if (rv == NNG_OK) {
                msg.release();
            }

            return rv;
        }

        // 发送原始消息
        // 参数：msg - 原始消息指针
        // 返回：操作结果，0 表示成功
        int send(nng_msg* msg) noexcept {
            return nng_sendmsg(_My_socket, msg, 0);
        }

        // 同步接收数据
        // 参数：data - 数据缓冲区，size - 数据大小指针，flags - 接收标志
        // 返回：操作结果，0 表示成功
        int recv(void* data, size_t* size, int flags) noexcept {
            return nng_recv(_My_socket, data, size, flags);
        }

        // 同步接收原始消息
        // 参数：msg - 存储消息的指针，flags - 接收标志，默认为 0
        // 返回：操作结果，0 表示成功
        int recv(nng_msg** msg, int flags = 0) noexcept {
            return nng_recvmsg(_My_socket, msg, flags);
        }

        // 同步接收消息
        // 参数：flags - 接收标志，默认为 0
        // 返回：接收到的 Msg 对象
        // 异常：若接收失败，抛出 Exception
        Msg recv(int flags = 0) noexcept(false) {
            nng_msg* msg = nullptr;
            int rv = nng_recvmsg(_My_socket, &msg, flags);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_recvmsg");
            }

            return Msg(msg);
        }

        // 同步接收消息到指定对象
        // 参数：msg - 存储接收消息的 Msg 对象
        // 返回：操作结果，0 表示成功
        int recv(Msg& msg) noexcept {
            nng_msg* m = nullptr;
            int rv = nng_recvmsg(_My_socket, &m, 0);
            if (rv != NNG_OK) {
                return rv;
            }

            msg = m;
            return rv;
        }

        // 转换为 nng_socket
        // 返回：当前管理的 nng_socket
        operator nng_socket() const noexcept {
            return _My_socket;
        }

    private:
        // 静态创建方法：创建指定协议的套接字
        // 参数：creator - 套接字创建函数
        // 返回：创建的 Socket 对象
        // 异常：若创建失败，抛出 Exception
        static Socket _Create(_Socket_creator_t creator) noexcept(false) {
            nng_socket s;
            auto rv = creator(&s);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_xxx_open");
            }
            return Socket(s);
        }

        nng_socket _My_socket = NNG_SOCKET_INITIALIZER;
    };
}