#pragma once

#include "nngSocket.h"
#include <string>
#include <string_view>

namespace nng
{
    // SocketOpt 类：NNG 套接字选项操作类，继承自 Socket
    // 用途：提供便捷的套接字选项设置和获取功能，支持各种传输层和协议特定的选项配置
    // 特性：
    // - 继承 Socket 的所有功能，提供完整的套接字操作能力
    // - 提供各种选项的便捷设置和获取方法，避免直接使用 NNG C API
    // - 直接返回操作结果，不抛出异常，便于错误处理
    // - HeaderOnly 模式，所有实现都在头文件中，便于集成和使用
    // - 使用 std::string_view 参数，提高性能，避免不必要的字符串拷贝
    // - 支持 TLS、TCP、IPC、WebSocket 等多种传输层的选项配置
    class SocketOpt : public Socket
    {
    public:
        // 继承 Socket 的构造函数
        using Socket::Socket;

        // Socket 名称选项
        // 设置套接字名称，用于调试和日志记录
        // 参数：name - 套接字名称
        // 返回：操作结果，0 表示成功
        int set_socket_name(std::string_view name) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_SOCKNAME, name.data());
        }

        // 获取套接字名称
        // 参数：name - 输出参数，存储获取到的套接字名称
        // 返回：操作结果，0 表示成功
        int get_socket_name(std::string& name) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_SOCKNAME, &value);
            if (rv == NNG_OK) {
                name = value;
                nng_strfree(value);
            }
            return rv;
        }

        // Raw 模式选项
        // 设置套接字为原始模式，绕过协议层的消息封装
        // 参数：raw - true 表示启用原始模式，false 表示禁用
        // 返回：操作结果，0 表示成功
        int set_raw(bool raw) noexcept {
            return nng_socket_set_bool(_My_socket, NNG_OPT_RAW, raw);
        }

        // 获取套接字的原始模式状态
        // 参数：raw - 输出参数，存储原始模式状态
        // 返回：操作结果，0 表示成功
        int get_raw(bool& raw) const noexcept {
            return nng_socket_get_bool(_My_socket, NNG_OPT_RAW, &raw);
        }

        // 协议选项
        // 设置套接字的协议 ID
        // 参数：protocol - 协议 ID
        // 返回：操作结果，0 表示成功
        int set_protocol(int protocol) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_PROTO, protocol);
        }

        // 获取套接字的协议 ID
        // 参数：protocol - 输出参数，存储协议 ID
        // 返回：操作结果，0 表示成功
        int get_protocol(int& protocol) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_PROTO, &protocol);
        }

        // 获取套接字的协议名称
        // 参数：name - 输出参数，存储协议名称
        // 返回：操作结果，0 表示成功
        int get_protocol_name(std::string& name) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_PROTONAME, &value);
            if (rv == NNG_OK) {
                name = value;
                nng_strfree(value);
            }
            return rv;
        }

        // Peer 选项
        // 设置对等方的协议 ID
        // 参数：peer - 对等方协议 ID
        // 返回：操作结果，0 表示成功
        int set_peer(int peer) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_PEER, peer);
        }

        // 获取对等方的协议 ID
        // 参数：peer - 输出参数，存储对等方协议 ID
        // 返回：操作结果，0 表示成功
        int get_peer(int& peer) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_PEER, &peer);
        }

        // 获取对等方的协议名称
        // 参数：name - 输出参数，存储对等方协议名称
        // 返回：操作结果，0 表示成功
        int get_peer_name(std::string& name) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_PEERNAME, &value);
            if (rv == NNG_OK) {
                name = value;
                nng_strfree(value);
            }
            return rv;
        }

        // 缓冲区选项
        // 设置接收缓冲区大小
        // 参数：size - 接收缓冲区大小（字节）
        // 返回：操作结果，0 表示成功
        int set_recv_buffer(size_t size) noexcept {
            return nng_socket_set_size(_My_socket, NNG_OPT_RECVBUF, size);
        }

        // 获取接收缓冲区大小
        // 参数：size - 输出参数，存储接收缓冲区大小
        // 返回：操作结果，0 表示成功
        int get_recv_buffer(size_t& size) const noexcept {
            return nng_socket_get_size(_My_socket, NNG_OPT_RECVBUF, &size);
        }

        // 设置发送缓冲区大小
        // 参数：size - 发送缓冲区大小（字节）
        // 返回：操作结果，0 表示成功
        int set_send_buffer(size_t size) noexcept {
            return nng_socket_set_size(_My_socket, NNG_OPT_SENDBUF, size);
        }

        // 获取发送缓冲区大小
        // 参数：size - 输出参数，存储发送缓冲区大小
        // 返回：操作结果，0 表示成功
        int get_send_buffer(size_t& size) const noexcept {
            return nng_socket_get_size(_My_socket, NNG_OPT_SENDBUF, &size);
        }

        // 文件描述符选项
        // 设置接收文件描述符
        // 参数：fd - 文件描述符
        // 返回：操作结果，0 表示成功
        int set_recv_fd(int fd) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_RECVFD, fd);
        }

        // 获取接收文件描述符
        // 参数：fd - 输出参数，存储接收文件描述符
        // 返回：操作结果，0 表示成功
        int get_recv_fd(int& fd) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_RECVFD, &fd);
        }

        // 设置发送文件描述符
        // 参数：fd - 文件描述符
        // 返回：操作结果，0 表示成功
        int set_send_fd(int fd) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_SENDFD, fd);
        }

        // 获取发送文件描述符
        // 参数：fd - 输出参数，存储发送文件描述符
        // 返回：操作结果，0 表示成功
        int get_send_fd(int& fd) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_SENDFD, &fd);
        }

        // 超时选项
        // 设置接收超时时间
        // 参数：timeout - 超时时间（毫秒）
        // 返回：操作结果，0 表示成功
        int set_recv_timeout(nng_duration timeout) noexcept {
            return nng_socket_set_ms(_My_socket, NNG_OPT_RECVTIMEO, timeout);
        }

        // 获取接收超时时间
        // 参数：timeout - 输出参数，存储接收超时时间
        // 返回：操作结果，0 表示成功
        int get_recv_timeout(nng_duration& timeout) const noexcept {
            return nng_socket_get_ms(_My_socket, NNG_OPT_RECVTIMEO, &timeout);
        }

        // 设置发送超时时间
        // 参数：timeout - 超时时间（毫秒）
        // 返回：操作结果，0 表示成功
        int set_send_timeout(nng_duration timeout) noexcept {
            return nng_socket_set_ms(_My_socket, NNG_OPT_SENDTIMEO, timeout);
        }

        // 获取发送超时时间
        // 参数：timeout - 输出参数，存储发送超时时间
        // 返回：操作结果，0 表示成功
        int get_send_timeout(nng_duration& timeout) const noexcept {
            return nng_socket_get_ms(_My_socket, NNG_OPT_SENDTIMEO, &timeout);
        }

        // 地址选项
        // 设置本地地址
        // 参数：addr - 本地地址字符串
        // 返回：操作结果，0 表示成功
        int set_local_address(std::string_view addr) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_LOCADDR, addr.data());
        }

        // 获取本地地址
        // 参数：addr - 输出参数，存储本地地址
        // 返回：操作结果，0 表示成功
        int get_local_address(std::string& addr) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_LOCADDR, &value);
            if (rv == NNG_OK) {
                addr = value;
                nng_strfree(value);
            }
            return rv;
        }

        // 设置远程地址
        // 参数：addr - 远程地址字符串
        // 返回：操作结果，0 表示成功
        int set_remote_address(std::string_view addr) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_REMADDR, addr.data());
        }

        // 获取远程地址
        // 参数：addr - 输出参数，存储远程地址
        // 返回：操作结果，0 表示成功
        int get_remote_address(std::string& addr) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_REMADDR, &value);
            if (rv == NNG_OK) {
                addr = value;
                nng_strfree(value);
            }
            return rv;
        }

        // URL 选项
        // 设置套接字的 URL
        // 参数：url - URL 字符串
        // 返回：操作结果，0 表示成功
        int set_url(std::string_view url) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_URL, url.data());
        }

        // 获取套接字的 URL
        // 参数：url - 输出参数，存储 URL
        // 返回：操作结果，0 表示成功
        int get_url(std::string& url) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_URL, &value);
            if (rv == NNG_OK) {
                url = value;
                nng_strfree(value);
            }
            return rv;
        }

        // TTL 选项
        // 设置最大 TTL（生存时间）
        // 参数：ttl - TTL 值
        // 返回：操作结果，0 表示成功
        int set_max_ttl(int ttl) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_MAXTTL, ttl);
        }

        // 获取最大 TTL
        // 参数：ttl - 输出参数，存储最大 TTL
        // 返回：操作结果，0 表示成功
        int get_max_ttl(int& ttl) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_MAXTTL, &ttl);
        }

        // 接收最大大小选项
        // 设置接收消息的最大大小
        // 参数：size - 最大消息大小（字节）
        // 返回：操作结果，0 表示成功
        int set_recv_max_size(size_t size) noexcept {
            return nng_socket_set_size(_My_socket, NNG_OPT_RECVMAXSZ, size);
        }

        // 获取接收消息的最大大小
        // 参数：size - 输出参数，存储最大消息大小
        // 返回：操作结果，0 表示成功
        int get_recv_max_size(size_t& size) const noexcept {
            return nng_socket_get_size(_My_socket, NNG_OPT_RECVMAXSZ, &size);
        }

        // 重连选项
        // 设置最小重连时间
        // 参数：time - 最小重连时间（毫秒）
        // 返回：操作结果，0 表示成功
        int set_reconnect_time_min(nng_duration time) noexcept {
            return nng_socket_set_ms(_My_socket, NNG_OPT_RECONNMINT, time);
        }

        // 获取最小重连时间
        // 参数：time - 输出参数，存储最小重连时间
        // 返回：操作结果，0 表示成功
        int get_reconnect_time_min(nng_duration& time) const noexcept {
            return nng_socket_get_ms(_My_socket, NNG_OPT_RECONNMINT, &time);
        }

        // 设置最大重连时间
        // 参数：time - 最大重连时间（毫秒）
        // 返回：操作结果，0 表示成功
        int set_reconnect_time_max(nng_duration time) noexcept {
            return nng_socket_set_ms(_My_socket, NNG_OPT_RECONNMAXT, time);
        }

        // 获取最大重连时间
        // 参数：time - 输出参数，存储最大重连时间
        // 返回：操作结果，0 表示成功
        int get_reconnect_time_max(nng_duration& time) const noexcept {
            return nng_socket_get_ms(_My_socket, NNG_OPT_RECONNMAXT, &time);
        }

        // TLS 选项
        // 设置 TLS 配置
        // 参数：config - TLS 配置对象指针
        // 返回：操作结果，0 表示成功
        int set_tls_config(nng_tls_config* config) noexcept {
            return nng_socket_set_ptr(_My_socket, NNG_OPT_TLS_CONFIG, config);
        }

        // 获取 TLS 配置
        // 参数：config - 输出参数，存储 TLS 配置对象指针
        // 返回：操作结果，0 表示成功
        int get_tls_config(nng_tls_config** config) const noexcept {
            return nng_socket_get_ptr(_My_socket, NNG_OPT_TLS_CONFIG, (void**)config);
        }

        // 设置 TLS 认证模式
        // 参数：mode - 认证模式（NNG_TLS_AUTH_MODE_*）
        // 返回：操作结果，0 表示成功
        int set_tls_auth_mode(int mode) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_TLS_AUTH_MODE, mode);
        }

        // 获取 TLS 认证模式
        // 参数：mode - 输出参数，存储认证模式
        // 返回：操作结果，0 表示成功
        int get_tls_auth_mode(int& mode) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_TLS_AUTH_MODE, &mode);
        }

        // 设置 TLS 证书和密钥文件
        // 参数：file - 证书和密钥文件路径
        // 返回：操作结果，0 表示成功
        int set_tls_cert_key_file(std::string_view file) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_TLS_CERT_KEY_FILE, file.data());
        }

        // 设置 TLS CA 文件
        // 参数：file - CA 证书文件路径
        // 返回：操作结果，0 表示成功
        int set_tls_ca_file(std::string_view file) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_TLS_CA_FILE, file.data());
        }

        // 设置 TLS 服务器名称
        // 参数：name - 服务器名称
        // 返回：操作结果，0 表示成功
        int set_tls_server_name(std::string_view name) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_TLS_SERVER_NAME, name.data());
        }

        // 获取 TLS 验证状态
        // 参数：verified - 输出参数，存储验证状态
        // 返回：操作结果，0 表示成功
        int get_tls_verified(bool& verified) const noexcept {
            return nng_socket_get_bool(_My_socket, NNG_OPT_TLS_VERIFIED, &verified);
        }

        // 获取 TLS 对等方通用名称
        // 参数：cn - 输出参数，存储对等方通用名称
        // 返回：操作结果，0 表示成功
        int get_tls_peer_cn(std::string& cn) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_TLS_PEER_CN, &value);
            if (rv == NNG_OK) {
                cn = value;
                nng_strfree(value);
            }
            return rv;
        }

        // 获取 TLS 对等方备用名称列表
        // 参数：names - 输出参数，存储对等方备用名称列表
        // 返回：操作结果，0 表示成功
        int get_tls_peer_alt_names(nng_list*& names) const noexcept {
            return nng_socket_get_list(_My_socket, NNG_OPT_TLS_PEER_ALT_NAMES, &names);
        }

        // TCP 选项
        // 设置 TCP nodelay 选项
        // 参数：nodelay - true 表示禁用 Nagle 算法，false 表示启用
        // 返回：操作结果，0 表示成功
        int set_tcp_nodelay(bool nodelay) noexcept {
            return nng_socket_set_bool(_My_socket, NNG_OPT_TCP_NODELAY, nodelay);
        }

        // 获取 TCP nodelay 选项状态
        // 参数：nodelay - 输出参数，存储 nodelay 状态
        // 返回：操作结果，0 表示成功
        int get_tcp_nodelay(bool& nodelay) const noexcept {
            return nng_socket_get_bool(_My_socket, NNG_OPT_TCP_NODELAY, &nodelay);
        }

        // 设置 TCP keepalive 选项
        // 参数：keepalive - true 表示启用 keepalive，false 表示禁用
        // 返回：操作结果，0 表示成功
        int set_tcp_keepalive(bool keepalive) noexcept {
            return nng_socket_set_bool(_My_socket, NNG_OPT_TCP_KEEPALIVE, keepalive);
        }

        // 获取 TCP keepalive 选项状态
        // 参数：keepalive - 输出参数，存储 keepalive 状态
        // 返回：操作结果，0 表示成功
        int get_tcp_keepalive(bool& keepalive) const noexcept {
            return nng_socket_get_bool(_My_socket, NNG_OPT_TCP_KEEPALIVE, &keepalive);
        }

        // 获取 TCP 绑定端口
        // 参数：port - 输出参数，存储绑定端口号
        // 返回：操作结果，0 表示成功
        int get_tcp_bound_port(int& port) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_TCP_BOUND_PORT, &port);
        }

        // IPC 选项
        // 设置 IPC 安全描述符（仅 Windows）
        // 参数：desc - 安全描述符指针
        // 返回：操作结果，0 表示成功
        int set_ipc_security_descriptor(void* desc) noexcept {
            return nng_socket_set_ptr(_My_socket, NNG_OPT_IPC_SECURITY_DESCRIPTOR, desc);
        }

        // 设置 IPC 权限（仅 POSIX）
        // 参数：permissions - 权限位
        // 返回：操作结果，0 表示成功
        int set_ipc_permissions(int permissions) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_IPC_PERMISSIONS, permissions);
        }

        // 获取 IPC 权限
        // 参数：permissions - 输出参数，存储权限位
        // 返回：操作结果，0 表示成功
        int get_ipc_permissions(int& permissions) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_IPC_PERMISSIONS, &permissions);
        }

        // 获取对等方用户 ID（仅 POSIX）
        // 参数：uid - 输出参数，存储用户 ID
        // 返回：操作结果，0 表示成功
        int get_peer_uid(int& uid) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_PEER_UID, &uid);
        }

        // 获取对等方组 ID（仅 POSIX）
        // 参数：gid - 输出参数，存储组 ID
        // 返回：操作结果，0 表示成功
        int get_peer_gid(int& gid) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_PEER_GID, &gid);
        }

        // 获取对等方进程 ID
        // 参数：pid - 输出参数，存储进程 ID
        // 返回：操作结果，0 表示成功
        int get_peer_pid(int& pid) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_PEER_PID, &pid);
        }

        // 获取对等方区域 ID（仅 SunOS）
        // 参数：zoneid - 输出参数，存储区域 ID
        // 返回：操作结果，0 表示成功
        int get_peer_zoneid(int& zoneid) const noexcept {
            return nng_socket_get_int(_My_socket, NNG_OPT_PEER_ZONEID, &zoneid);
        }

        // WebSocket 选项
        // 设置 WebSocket 请求头
        // 参数：headers - 请求头字符串（CRLF 分隔）
        // 返回：操作结果，0 表示成功
        int set_ws_request_headers(std::string_view headers) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_WS_REQUEST_HEADERS, headers.data());
        }

        // 获取 WebSocket 请求头
        // 参数：headers - 输出参数，存储请求头
        // 返回：操作结果，0 表示成功
        int get_ws_request_headers(std::string& headers) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_WS_REQUEST_HEADERS, &value);
            if (rv == NNG_OK) {
                headers = value;
                nng_strfree(value);
            }
            return rv;
        }

        // 设置 WebSocket 响应头
        // 参数：headers - 响应头字符串（CRLF 分隔）
        // 返回：操作结果，0 表示成功
        int set_ws_response_headers(std::string_view headers) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_WS_RESPONSE_HEADERS, headers.data());
        }

        // 获取 WebSocket 响应头
        // 参数：headers - 输出参数，存储响应头
        // 返回：操作结果，0 表示成功
        int get_ws_response_headers(std::string& headers) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_WS_RESPONSE_HEADERS, &value);
            if (rv == NNG_OK) {
                headers = value;
                nng_strfree(value);
            }
            return rv;
        }

        // 获取 WebSocket 请求头中的特定字段
        // 参数：name - 头部字段名称，value - 输出参数，存储字段值
        // 返回：操作结果，0 表示成功
        int get_ws_request_header(std::string_view name, std::string& value) const noexcept {
            char* val;
            int rv = nng_socket_get_string(_My_socket, (std::string(NNG_OPT_WS_REQUEST_HEADER) + std::string(name)).c_str(), &val);
            if (rv == NNG_OK) {
                value = val;
                nng_strfree(val);
            }
            return rv;
        }

        // 获取 WebSocket 响应头中的特定字段
        // 参数：name - 头部字段名称，value - 输出参数，存储字段值
        // 返回：操作结果，0 表示成功
        int get_ws_response_header(std::string_view name, std::string& value) const noexcept {
            char* val;
            int rv = nng_socket_get_string(_My_socket, (std::string(NNG_OPT_WS_RESPONSE_HEADER) + std::string(name)).c_str(), &val);
            if (rv == NNG_OK) {
                value = val;
                nng_strfree(val);
            }
            return rv;
        }

        // 获取 WebSocket 请求 URI
        // 参数：uri - 输出参数，存储请求 URI
        // 返回：操作结果，0 表示成功
        int get_ws_request_uri(std::string& uri) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_WS_REQUEST_URI, &value);
            if (rv == NNG_OK) {
                uri = value;
                nng_strfree(value);
            }
            return rv;
        }

        // 设置 WebSocket 发送最大帧大小
        // 参数：size - 最大帧大小（字节）
        // 返回：操作结果，0 表示成功
        int set_ws_send_max_frame(size_t size) noexcept {
            return nng_socket_set_size(_My_socket, NNG_OPT_WS_SENDMAXFRAME, size);
        }

        // 获取 WebSocket 发送最大帧大小
        // 参数：size - 输出参数，存储最大帧大小
        // 返回：操作结果，0 表示成功
        int get_ws_send_max_frame(size_t& size) const noexcept {
            return nng_socket_get_size(_My_socket, NNG_OPT_WS_SENDMAXFRAME, &size);
        }

        // 设置 WebSocket 接收最大帧大小
        // 参数：size - 最大帧大小（字节）
        // 返回：操作结果，0 表示成功
        int set_ws_recv_max_frame(size_t size) noexcept {
            return nng_socket_set_size(_My_socket, NNG_OPT_WS_RECVMAXFRAME, size);
        }

        // 获取 WebSocket 接收最大帧大小
        // 参数：size - 输出参数，存储最大帧大小
        // 返回：操作结果，0 表示成功
        int get_ws_recv_max_frame(size_t& size) const noexcept {
            return nng_socket_get_size(_My_socket, NNG_OPT_WS_RECVMAXFRAME, &size);
        }

        // 设置 WebSocket 协议
        // 参数：protocol - 协议名称
        // 返回：操作结果，0 表示成功
        int set_ws_protocol(std::string_view protocol) noexcept {
            return nng_socket_set_string(_My_socket, NNG_OPT_WS_PROTOCOL, protocol.data());
        }

        // 获取 WebSocket 协议
        // 参数：protocol - 输出参数，存储协议名称
        // 返回：操作结果，0 表示成功
        int get_ws_protocol(std::string& protocol) const noexcept {
            char* value;
            int rv = nng_socket_get_string(_My_socket, NNG_OPT_WS_PROTOCOL, &value);
            if (rv == NNG_OK) {
                protocol = value;
                nng_strfree(value);
            }
            return rv;
        }

        // 设置 WebSocket 发送文本模式
        // 参数：text - true 表示发送文本帧，false 表示发送二进制帧
        // 返回：操作结果，0 表示成功
        int set_ws_send_text(bool text) noexcept {
            return nng_socket_set_bool(_My_socket, NNG_OPT_WS_SEND_TEXT, text);
        }

        // 获取 WebSocket 发送文本模式状态
        // 参数：text - 输出参数，存储文本模式状态
        // 返回：操作结果，0 表示成功
        int get_ws_send_text(bool& text) const noexcept {
            return nng_socket_get_bool(_My_socket, NNG_OPT_WS_SEND_TEXT, &text);
        }

        // 设置 WebSocket 接收文本模式
        // 参数：text - true 表示接收文本帧，false 表示接收二进制帧
        // 返回：操作结果，0 表示成功
        int set_ws_recv_text(bool text) noexcept {
            return nng_socket_set_bool(_My_socket, NNG_OPT_WS_RECV_TEXT, text);
        }

        // 获取 WebSocket 接收文本模式状态
        // 参数：text - 输出参数，存储文本模式状态
        // 返回：操作结果，0 表示成功
        int get_ws_recv_text(bool& text) const noexcept {
            return nng_socket_get_bool(_My_socket, NNG_OPT_WS_RECV_TEXT, &text);
        }

        // Socket FD 选项
        // 设置套接字文件描述符
        // 参数：fd - 文件描述符
        // 返回：操作结果，0 表示成功
        int set_socket_fd(int fd) noexcept {
            return nng_socket_set_int(_My_socket, NNG_OPT_SOCKET_FD, fd);
        }
    };
} 