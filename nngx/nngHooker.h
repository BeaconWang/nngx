#pragma once

#include "nngException.h"

namespace nng
{
    // Hooker 类：为 NNG 连接器提供钩子功能的模板类
    // 用途：允许在特定操作（如地址处理或启动连接器）前执行用户定义的回调函数
    // 特性：
    // - 提供静态方法设置地址处理和启动前的回调函数
    // - 使用互斥锁确保线程安全的回调设置和调用
    // - 支持模板参数，适用于不同类型的连接器（如 Listener 或 Dialer）
    template < class _TyConnector>
    class Hooker
    {
    public:
        // 定义回调函数类型：用于预处理地址
        using _Pre_address_t = std::function<std::string(std::string_view sv)>;
        // 定义回调函数类型：用于在启动连接器前执行
        using _Pre_start_t = std::function<void(_TyConnector& _Connector_ref)>;
    public:
        // 设置地址预处理回调函数
        // 参数：_Callback - 用户定义的地址处理回调
        // 说明：线程安全，通过互斥锁保护回调设置
        static void set_pre_address(_Pre_address_t _Callback) noexcept {
            std::lock_guard<std::mutex> locker(_My_mtx);
            _My_pre_address = _Callback;
        }

        // 设置启动前预处理回调函数
        // 参数：_Callback - 用户定义的启动前回调
        // 说明：线程安全，通过互斥锁保护回调设置
        static void set_pre_start(_Pre_start_t _Callback) noexcept {
            std::lock_guard<std::mutex> locker(_My_mtx);
            _My_pre_start = _Callback;
        }
    protected:
        // 预处理地址
        // 参数：_Address - 输入的地址字符串视图
        // 返回：处理后的地址字符串
        // 说明：如果设置了回调，则调用回调处理地址；否则返回原始地址
        static std::string _Pre_address(std::string_view _Address) noexcept {
            std::lock_guard<std::mutex> locker(_My_mtx);
            if (_My_pre_address) {
                return _My_pre_address(_Address);
            }

            return _Address.data();
        }

        // 在启动连接器前执行预处理
        // 参数：_Connector_ref - 连接器对象的引用
        // 说明：如果设置了回调，则调用回调进行预处理
        static void _Pre_start(_TyConnector& _Connector_ref) noexcept {
            std::lock_guard<std::mutex> locker(_My_mtx);
            if (_My_pre_start) {
                _My_pre_start(_Connector_ref);
            }
        }
    private:
        // 静态互斥锁：保护回调函数的设置和调用
        inline static std::mutex _My_mtx;
        // 静态地址预处理回调
        inline static _Pre_address_t _My_pre_address;
        // 静态启动前预处理回调
        inline static _Pre_start_t _My_pre_start;
    };
}