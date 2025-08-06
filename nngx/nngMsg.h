#pragma once

#include <cstring>

#include "nngException.h"

namespace nng
{
    // Msg 类：NNG 消息（nng_msg）的 C++ RAII 包装类
    // 用途：提供对 NNG 消息的便捷管理，支持消息的创建、修改、查询和销毁
    // 特性：
    // - 使用 RAII 管理 nng_msg 资源，确保自动释放
    // - 支持移动构造和移动赋值，禁用拷贝以保证资源独占
    // - 提供对消息头部和正文的追加、插入、裁剪等操作
    // - 支持无符号整数和字符串的便捷操作
    // - 异常安全：分配失败或操作错误时抛出 Exception
    class Msg
    {
    public:
        // 构造函数：使用现有的 nng_msg 指针初始化消息对象
        // 参数：msg - nng_msg 指针，默认为 nullptr
        explicit Msg(nng_msg* msg = nullptr) noexcept : _My_msg(msg) {
        }

        // 构造函数：创建指定初始大小的空消息
        // 参数：size - 消息的初始大小
        // 异常：若分配失败，抛出 Exception
        template <
            typename T,
            std::enable_if_t<std::is_integral_v<T>, int> = 0
        >
        explicit Msg(T size) noexcept(false) {
            int rv = nng_msg_alloc(&_My_msg, (size_t)size);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_alloc");
            }
        }

        // 构造函数：使用指定数据和大小创建消息
        // 参数：data - 数据指针，data_size - 数据大小
        // 异常：若分配失败，抛出 Exception
        explicit Msg(const void* data, size_t data_size) noexcept(false) {
            int rv = nng_msg_alloc(&_My_msg, data_size);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_alloc");
            }
            std::memcpy(body(), data, data_size);
        }

        // 构造函数：使用 nng_iov 结构创建消息
        // 参数：iov - 包含数据缓冲区和长度的 nng_iov 结构
        // 异常：若分配失败，抛出 Exception
        explicit Msg(const nng_iov& iov) noexcept(false) {
            int rv = nng_msg_alloc(&_My_msg, iov.iov_len);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_alloc");
            }
            std::memcpy(body(), iov.iov_buf, iov.iov_len);
        }

        // 析构函数：释放消息资源
        ~Msg() noexcept {
            if (_My_msg) {
                nng_msg_free(_My_msg);
            }
        }

        // 移动构造函数：转移消息所有权
        // 参数：other - 源消息对象
        Msg(Msg&& other) noexcept : _My_msg(other._My_msg) {
            other._My_msg = nullptr;
        }

        // 移动赋值运算符：转移消息所有权
        // 参数：other - 源消息对象
        // 返回：当前对象的引用
        Msg& operator=(Msg&& other) noexcept {
            if (this != &other) {
                if (_My_msg) {
                    nng_msg_free(_My_msg);
                }
                _My_msg = other._My_msg;
                other._My_msg = nullptr;
            }
            return *this;
        }

        // 禁用拷贝构造函数
        Msg(const Msg&) = delete;

        // 禁用拷贝赋值运算符
        Msg& operator=(const Msg&) = delete;

        // 重新分配消息大小
        // 参数：size - 新的消息大小
        // 返回：操作结果，0 表示成功
        // 异常：可能抛出分配失败的异常
        int realloc(size_t size) noexcept {
            if (_My_msg == nullptr) {
                return nng_msg_alloc(&_My_msg, size);
            }
            else {
                return nng_msg_realloc(_My_msg, size);
            }
        }

        // 预留消息空间
        // 参数：size - 预留的空间大小
        // 返回：操作结果，0 表示成功
        int reserve(size_t size) noexcept {
            return nng_msg_reserve(_My_msg, size);
        }

        // 获取消息当前容量
        // 返回：消息的容量大小
        size_t capacity() const noexcept {
            return nng_msg_capacity(_My_msg);
        }

        // 获取消息头部指针
        // 返回：指向消息头部的指针
        void* header() const noexcept {
            return nng_msg_header(_My_msg);
        }

        // 获取消息头部长度
        // 返回：消息头部的长度
        size_t header_len() const noexcept {
            return nng_msg_header_len(_My_msg);
        }

        // 获取消息正文指针
        // 返回：指向消息正文的指针
        void* body() const noexcept {
            return nng_msg_body(_My_msg);
        }

        // 获取消息正文长度
        // 返回：消息正文的长度
        size_t len() const noexcept {
            return nng_msg_len(_My_msg);
        }

        // 获取距离消息正文尾部 offset 字节处的数据指针
        // 用途：便捷访问消息正文末尾附近的数据（如协议尾部字段等）
        // 参数：offset - 距离正文尾部的字节偏移（0 表示最后一个字节）
        // 返回：指向该偏移处的指针，若 offset 超过正文长度则返回 nullptr
        void* body_tail(size_t offset) const noexcept {
            size_t l = len();
            if (offset > l) return nullptr;
            return static_cast<uint8_t*>(body()) + (l - offset);
        }

        // 向消息正文追加数据
        // 参数：data - 数据指针，data_size - 数据大小
        // 返回：操作结果，0 表示成功
        inline int push(const void* data, size_t data_size) noexcept { return append(data, data_size); }
        int append(const void* data, size_t data_size) noexcept {
            return nng_msg_append(_My_msg, data, data_size);
        }

        // 在消息正文开头插入数据
        // 参数：data - 数据指针，data_size - 数据大小
        // 返回：操作结果，0 表示成功
        int insert(const void* data, size_t data_size) noexcept {
            return nng_msg_insert(_My_msg, data, data_size);
        }

        // 从消息正文开头裁剪指定大小的数据
        // 参数：size - 要裁剪的数据大小
        // 返回：操作结果，0 表示成功
        int trim(size_t size) noexcept {
            return nng_msg_trim(_My_msg, size);
        }

        // 从消息正文末尾裁剪指定大小的数据
        // 参数：size - 要裁剪的数据大小
        // 返回：操作结果，0 表示成功
        inline int pop(size_t size) noexcept { return chop(size); }
        int chop(size_t size) noexcept {
            return nng_msg_chop(_My_msg, size);
        }

        // 向消息头部追加数据
        // 参数：data - 数据指针，data_size - 数据大小
        // 返回：操作结果，0 表示成功
        int header_append(const void* data, size_t data_size) noexcept {
            return nng_msg_header_append(_My_msg, data, data_size);
        }

        // 在消息头部开头插入数据
        // 参数：data - 数据指针，data_size - 数据大小
        // 返回：操作结果，0 表示成功
        int header_insert(const void* data, size_t data_size) noexcept {
            return nng_msg_header_insert(_My_msg, data, data_size);
        }

        // 从消息头部开头裁剪指定大小的数据
        // 参数：size - 要裁剪的数据大小
        // 返回：操作结果，0 表示成功
        int header_trim(size_t size) noexcept {
            return nng_msg_header_trim(_My_msg, size);
        }

        // 从消息头部末尾裁剪指定大小的数据
        // 参数：size - 要裁剪的数据大小
        // 返回：操作结果，0 表示成功
        int header_chop(size_t size) noexcept {
            return nng_msg_header_chop(_My_msg, size);
        }

        // 向消息头部追加 16 位无符号整数
        // 参数：val - 要追加的 16 位无符号整数
        // 返回：操作结果，0 表示成功
        int header_append_u16(uint16_t val) noexcept {
            return nng_msg_header_append_u16(_My_msg, val);
        }

        // 向消息头部追加 32 位无符号整数
        // 参数：val - 要追加的 32 位无符号整数
        // 返回：操作结果，0 表示成功
        int header_append_u32(uint32_t val) noexcept {
            return nng_msg_header_append_u32(_My_msg, val);
        }

        // 向消息头部追加 64 位无符号整数
        // 参数：val - 要追加的 64 位无符号整数
        // 返回：操作结果，0 表示成功
        int header_append_u64(uint64_t val) noexcept {
            return nng_msg_header_append_u64(_My_msg, val);
        }

        // 在消息头部开头插入 16 位无符号整数
        // 参数：val - 要插入的 16 位无符号整数
        // 返回：操作结果，0 表示成功
        int header_insert_u16(uint16_t val) noexcept {
            return nng_msg_header_insert_u16(_My_msg, val);
        }

        // 在消息头部开头插入 32 位无符号整数
        // 参数：val - 要插入的 32 位无符号整数
        // 返回：操作结果，0 表示成功
        int header_insert_u32(uint32_t val) noexcept {
            return nng_msg_header_insert_u32(_My_msg, val);
        }

        // 在消息头部开头插入 64 位无符号整数
        // 参数：val - 要插入的 64 位无符号整数
        // 返回：操作结果，0 表示成功
        int header_insert_u64(uint64_t val) noexcept {
            return nng_msg_header_insert_u64(_My_msg, val);
        }

        // 从消息头部末尾裁剪 16 位无符号整数
        // 参数：val - 存储裁剪出的 16 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int header_chop_u16(uint16_t* val) noexcept {
            return nng_msg_header_chop_u16(_My_msg, val);
        }

        // 从消息头部末尾裁剪 16 位无符号整数
        // 返回：裁剪出的 16 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint16_t header_chop_u16() noexcept(false) {
            uint16_t v;
            int rv = nng_msg_header_chop_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_chop_u16");
            }
            return v;
        }

        // 从消息头部末尾裁剪 32 位无符号整数
        // 参数：val - 存储裁剪出的 32 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int header_chop_u32(uint32_t* val) noexcept {
            return nng_msg_header_chop_u32(_My_msg, val);
        }

        // 从消息头部末尾裁剪 32 位无符号整数
        // 返回：裁剪出的 32 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint32_t header_chop_u32() noexcept(false) {
            uint32_t v;
            int rv = nng_msg_header_chop_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_chop_u32");
            }
            return v;
        }

        // 从消息头部末尾裁剪 64 位无符号整数
        // 参数：val - 存储裁剪出的 64 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int header_chop_u64(uint64_t* val) noexcept {
            return nng_msg_header_chop_u64(_My_msg, val);
        }

        // 从消息头部末尾裁剪 64 位无符号整数
        // 返回：裁剪出的 64 位无符号整数（以 optional 形式）
        // 异常：若操作失败，抛出 Exception
        std::optional<uint64_t> header_chop_u64() noexcept(false) {
            uint64_t v;
            int rv = nng_msg_header_chop_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_chop_u64");
            }
            return v;
        }

        // 从消息头部开头裁剪 16 位无符号整数
        // 参数：val - 存储裁剪出的 16 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int header_trim_u16(uint16_t* val) noexcept {
            return nng_msg_header_trim_u16(_My_msg, val);
        }

        // 从消息头部开头裁剪 16 位无符号整数
        // 返回：裁剪出的 16 位无符号整数（以 optional 形式）
        // 异常：若操作失败，抛出 Exception
        std::optional<uint16_t> header_trim_u16() noexcept(false) {
            uint16_t v;
            int rv = nng_msg_header_trim_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_trim_u16");
            }
            return v;
        }

        // 从消息头部开头裁剪 32 位无符号整数
        // 参数：val - 存储裁剪出的 32 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int header_trim_u32(uint32_t* val) noexcept {
            return nng_msg_header_trim_u32(_My_msg, val);
        }

        // 从消息头部开头裁剪 32 位无符号整数
        // 返回：裁剪出的 32 位无符号整数（以 optional 形式）
        // 异常：若操作失败，抛出 Exception
        std::optional<uint32_t> header_trim_u32() noexcept(false) {
            uint32_t v;
            int rv = nng_msg_header_trim_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_trim_u32");
            }
            return v;
        }

        // 从消息头部开头裁剪 64 位无符号整数
        // 参数：val - 存储裁剪出的 64 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int header_trim_u64(uint64_t* val) noexcept {
            return nng_msg_header_trim_u64(_My_msg, val);
        }

        // 从消息头部开头裁剪 64 位无符号整数
        // 返回：裁剪出的 64 位无符号整数（以 optional 形式）
        // 异常：若操作失败，抛出 Exception
        std::optional<uint64_t> header_trim_u64() noexcept(false) {
            uint64_t v;
            int rv = nng_msg_header_trim_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_trim_u64");
            }
            return v;
        }

        // 向消息正文追加 16 位无符号整数
        // 参数：val - 要追加的 16 位无符号整数
        // 返回：操作结果，0 表示成功
        inline int push_u16(uint16_t val) noexcept { return append_u16(val); }
        int append_u16(uint16_t val) noexcept {
            return nng_msg_append_u16(_My_msg, val);
        }

        // 向消息正文追加 32 位无符号整数
        // 参数：val - 要追加的 32 位无符号整数
        // 返回：操作结果，0 表示成功
        inline int push_u32(uint32_t val) noexcept { return append_u32(val); }
        int append_u32(uint32_t val) noexcept {
            return nng_msg_append_u32(_My_msg, val);
        }

        // 向消息正文追加 64 位无符号整数
        // 参数：val - 要追加的 64 位无符号整数
        // 返回：操作结果，0 表示成功
        inline int push_u64(uint64_t val) noexcept { return append_u64(val); }
        int append_u64(uint64_t val) noexcept {
            return nng_msg_append_u64(_My_msg, val);
        }

        // 向消息正文追加字符串
        // 参数：sv - 要追加的字符串视图
        // 返回：操作结果，0 表示成功
        inline int push_string(std::string_view sv) noexcept { return append_string(sv); }
        int append_string(std::string_view sv) noexcept {
            int rv = nng_msg_append(_My_msg, sv.data(), sv.size());
            if (rv != NNG_OK) {
                return rv;
            }
            return nng_msg_append_u32(_My_msg, (uint32_t)sv.size());
        }

        // 在消息正文开头插入 16 位无符号整数
        // 参数：val - 要插入的 16 位无符号整数
        // 返回：操作结果，0 表示成功
        int insert_u16(uint16_t val) noexcept {
            return nng_msg_insert_u16(_My_msg, val);
        }

        // 在消息正文开头插入 32 位无符号整数
        // 参数：val - 要插入的 32 位无符号整数
        // 返回：操作结果，0 表示成功
        int insert_u32(uint32_t val) noexcept {
            return nng_msg_insert_u32(_My_msg, val);
        }

        // 在消息正文开头插入 64 位无符号整数
        // 参数：val - 要插入的 64 位无符号整数
        // 返回：操作结果，0 表示成功
        int insert_u64(uint64_t val) noexcept {
            return nng_msg_insert_u64(_My_msg, val);
        }

        // 在消息正文开头插入字符串
        // 参数：sv - 要插入的字符串视图
        // 返回：操作结果，0 表示成功
        int insert_string(std::string_view sv) noexcept {
            int rv = nng_msg_insert(_My_msg, sv.data(), sv.size());
            if (rv != NNG_OK) {
                return rv;
            }
            return nng_msg_insert_u32(_My_msg, (uint32_t)sv.size());
        }

        // 从消息正文末尾裁剪 16 位无符号整数
        // 参数：val - 存储裁剪出的 16 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        inline int pop_u16(uint16_t* val) noexcept { return chop_u16(val); }
        int chop_u16(uint16_t* val) noexcept {
            return nng_msg_chop_u16(_My_msg, val);
        }

        // 从消息正文末尾裁剪 16 位无符号整数
        // 返回：裁剪出的 16 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint16_t chop_u16() noexcept(false) {
            uint16_t v;
            auto rv = nng_msg_chop_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_chop_u16");
            }
            return v;
        }
        inline std::optional<uint16_t> pop_u16() noexcept {
            try {
                return chop_u16();
            }
            catch (...) {
                return std::nullopt;
            }
        }

        // 从消息正文末尾裁剪 32 位无符号整数
        // 参数：val - 存储裁剪出的 32 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        inline int pop_u32(uint32_t* val) noexcept { return chop_u32(val); }
        int chop_u32(uint32_t* val) noexcept {
            return nng_msg_chop_u32(_My_msg, val);
        }

        // 从消息正文末尾裁剪 32 位无符号整数
        // 返回：裁剪出的 32 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint32_t chop_u32() noexcept(false) {
            uint32_t v;
            auto rv = nng_msg_chop_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_chop_u32");
            }
            return v;
        }
        inline std::optional<uint32_t> pop_u32() noexcept {
            try {
                return chop_u32();
            }
            catch (...) {
                return std::nullopt;
            }
        }

        // 从消息正文末尾裁剪 64 位无符号整数
        // 参数：val - 存储裁剪出的 64 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        inline int pop_u64(uint64_t* val) noexcept { return chop_u64(val); }
        int chop_u64(uint64_t* val) noexcept {
            return nng_msg_chop_u64(_My_msg, val);
        }

        // 从消息正文末尾裁剪 64 位无符号整数
        // 返回：裁剪出的 64 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint64_t chop_u64() noexcept(false) {
            uint64_t v;
            auto rv = nng_msg_chop_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_chop_u64");
            }
            return v;
        }
        inline std::optional<uint64_t> pop_u64() noexcept {
            try {
                return chop_u64();
            }
            catch (...) {
                return std::nullopt;
            }
        }

        // 从消息正文末尾裁剪字符串
        // 参数：s - 存储裁剪出的字符串
        // 返回：操作结果，0 表示成功
        inline int pop_string(std::string& s) noexcept { return chop_string(s); }
        int chop_string(std::string& s) noexcept {
            uint32_t string_len;
            int rv = nng_msg_chop_u32(_My_msg, &string_len);
            if (rv != NNG_OK) {
                return rv;
            }
            s.resize((size_t)string_len);
            std::memcpy(s.data(), (uint8_t*)nng_msg_body(_My_msg) + nng_msg_len(_My_msg) - string_len, string_len);
            return nng_msg_chop(_My_msg, string_len);
        }

        // 从消息正文末尾裁剪字符串
        // 返回：裁剪出的字符串
        // 异常：若操作失败，抛出 Exception
        std::string chop_string() noexcept(false) {
            std::string s;
            auto rv = chop_string(s);
            if (rv != NNG_OK) {
                throw Exception(rv, "chop_string");
            }
            return s;
        }
        inline std::optional<std::string> pop_string() noexcept {
            try {
                return chop_string();
            }
            catch (...) {
                return std::nullopt;
            }
        }

        // 从消息正文开头裁剪 16 位无符号整数
        // 参数：val - 存储裁剪出的 16 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int trim_u16(uint16_t* val) noexcept {
            return nng_msg_trim_u16(_My_msg, val);
        }

        // 从消息正文开头裁剪 16 位无符号整数
        // 返回：裁剪出的 16 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint16_t trim_u16() noexcept(false) {
            uint16_t v;
            auto rv = nng_msg_trim_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_trim_u16");
            }
            return v;
        }

        // 从消息正文开头裁剪 32 位无符号整数
        // 参数：val - 存储裁剪出的 32 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int trim_u32(uint32_t* val) noexcept {
            return nng_msg_trim_u32(_My_msg, val);
        }

        // 从消息正文开头裁剪 32 位无符号整数
        // 返回：裁剪出的 32 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint32_t trim_u32() noexcept(false) {
            uint32_t v;
            auto rv = nng_msg_trim_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_trim_u32");
            }
            return v;
        }

        // 从消息正文开头裁剪 64 位无符号整数
        // 参数：val - 存储裁剪出的 64 位无符号整数的指针
        // 返回：操作结果，0 表示成功
        int trim_u64(uint64_t* val) noexcept {
            return nng_msg_trim_u64(_My_msg, val);
        }

        // 从消息正文开头裁剪 64 位无符号整数
        // 返回：裁剪出的 64 位无符号整数
        // 异常：若操作失败，抛出 Exception
        uint64_t trim_u64() noexcept(false) {
            uint64_t v;
            auto rv = nng_msg_trim_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_trim_u64");
            }
            return v;
        }

        // 从消息正文开头裁剪字符串
        // 参数：s - 存储裁剪出的字符串
        // 返回：操作结果，0 表示成功
        int trim_string(std::string& s) noexcept {
            uint32_t string_len;
            int rv = nng_msg_trim_u32(_My_msg, &string_len);
            if (rv != NNG_OK) {
                return rv;
            }
            s.resize((size_t)string_len);
            std::memcpy(s.data(), nng_msg_body(_My_msg), string_len);
            return nng_msg_trim(_My_msg, string_len);
        }

        // 从消息正文开头裁剪字符串
        // 返回：裁剪出的字符串
        // 异常：若操作失败，抛出 Exception
        std::string trim_string() noexcept(false) {
            std::string s;
            int rv = trim_string(s);
            if (rv != NNG_OK) {
                throw Exception(rv, "trim_string");
            }
            return s;
        }

        // 复制消息到目标消息对象
        // 参数：dest - 目标消息对象
        // 返回：操作结果，0 表示成功
        int dup(Msg* dest) const noexcept {
            nng_msg* new_msg = nullptr;
            int rv = nng_msg_dup(&new_msg, _My_msg);
            if (rv == NNG_OK) {
                if (dest->_My_msg) {
                    nng_msg_free(dest->_My_msg);
                }
                dest->_My_msg = new_msg;
            }
            return rv;
        }

        // 清空消息正文
        void clear() noexcept {
            nng_msg_clear(_My_msg);
        }

        // 清空消息头部
        void header_clear() noexcept {
            nng_msg_header_clear(_My_msg);
        }

        // 设置关联的管道
        // 参数：pipe - 要设置的管道
        void set_pipe(nng_pipe pipe) noexcept {
            nng_msg_set_pipe(_My_msg, pipe);
        }

        // 获取关联的管道
        // 返回：关联的管道
        nng_pipe get_pipe() const noexcept {
            return nng_msg_get_pipe(_My_msg);
        }

        // 赋值运算符：设置新的 nng_msg 指针
        // 参数：msg - 新的 nng_msg 指针
        // 返回：当前对象的引用
        Msg& operator =(nng_msg* msg) noexcept {
            if (_My_msg != msg) {
                if (_My_msg) {
                    nng_msg_free(_My_msg);
                }
                _My_msg = msg;
            }
            return *this;
        }

        // 释放消息所有权
        // 返回：当前管理的 nng_msg 指针
        nng_msg* release() noexcept {
            return std::exchange(_My_msg, nullptr);
        }

        // 检查消息是否有效
        // 返回：true 表示消息有效，false 表示无效
        bool valid() const noexcept {
            return _My_msg != nullptr;
        }

        // 转换为 nng_msg 指针
        // 返回：当前管理的 nng_msg 指针
        operator nng_msg* () const noexcept { return _My_msg; }

        // 检查消息是否有效（布尔转换）
        // 返回：true 表示消息有效，false 表示无效
        operator bool() const noexcept { return valid(); }

    public:
        typedef uint64_t _Ty_msg_code;
        typedef _Ty_msg_code _Ty_msg_result;

        // 从消息裁剪消息代码
        // 参数：m - 消息对象
        // 返回：裁剪出的消息代码
        // 异常：若操作失败，抛出 Exception
        static inline _Ty_msg_code _Chop_msg_code(Msg& m) noexcept(false) {
            return m.chop_u64();
        }

        // 向消息追加消息代码
        // 参数：m - 消息对象，code - 要追加的消息代码
        static inline void _Append_msg_code(Msg& m, _Ty_msg_code code) noexcept {
            m.append_u64(code);
        }

        // 从消息裁剪消息结果
        // 参数：m - 消息对象
        // 返回：裁剪出的消息结果
        // 异常：若操作失败，抛出 Exception
        static inline _Ty_msg_result _Chop_msg_result(Msg& m) noexcept(false) {
            return m.chop_u64();
        }

        // 向消息追加消息结果
        // 参数：m - 消息对象，result - 要追加的消息结果
        static inline void _Append_msg_result(Msg& m, _Ty_msg_result result) noexcept {
            m.append_u64(result);
        }

    public:
        inline static Msg to_msg(std::string_view sv) noexcept(false) {
            return Msg(sv.data(), sv.size());
        }

        inline static std::string to_string(const Msg& m) {
            return std::string((const char*)m.body(), m.len());
        }
    private:
        nng_msg* _My_msg = nullptr;
    };
}