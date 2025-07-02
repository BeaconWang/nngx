#pragma once

#include <memory>
#include <string_view>

#include "nngException.h"
#include "nngSocket.h"
#include "nngListener.h"
#include "nngDialer.h"

namespace nng
{
    // Peer 类：NNG 套接字的模板类，继承 Socket，支持 Listener 或 Dialer 连接器
    // 用途：提供套接字的创建、连接启动和连接器管理，支持 Listener 或 Dialer 类型的连接
    // 特性：
    // - 使用 RAII 通过 std::unique_ptr 管理连接器资源
    // - 支持模板化连接器类型（Listener 或 Dialer），通过 static_assert 限制
    // - 提供虚函数接口以支持子类扩展
    // - 异常安全：连接器创建失败时可能抛出 Exception
    template<class _Connector_t = Listener>
    class Peer : virtual public Socket
    {
        static_assert(
            std::is_same_v<_Connector_t, Listener> || std::is_same_v<_Connector_t, Dialer>,
            "_Connector_t must be either Listener or Dialer"
            );

        // 创建套接字的虚函数，由子类实现
        // 返回：操作结果，0 表示成功
        virtual int _Create() noexcept = 0;

    public:
        // 启动连接
        // 参数：addr - 连接地址，flags - 启动标志，默认为 0
        // 返回：操作结果，0 表示成功
        // 异常：若创建套接字或连接器失败，抛出 Exception
        int start(std::string_view addr, int flags = 0) noexcept(false) {
            int rv = _Create();
            if (rv != NNG_OK) {
                return rv;
            }

            _My_connector = std::make_unique<_Connector_t>(*this, addr);
            return _My_connector->start(flags);
        }

        // 获取连接器指针
        // 返回：指向连接器的原始指针
        _Connector_t* get_connector() const noexcept { return _My_connector.get(); }

    protected:
        std::unique_ptr<_Connector_t> _My_connector;
    };
}