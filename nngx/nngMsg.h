#pragma once

#include <cstring>

#include "nngException.h"

namespace nng
{
    // Msg �ࣺNNG ��Ϣ��nng_msg���� C++ RAII ��װ��
    // ��;���ṩ�� NNG ��Ϣ�ı�ݹ���֧����Ϣ�Ĵ������޸ġ���ѯ������
    // ���ԣ�
    // - ʹ�� RAII ���� nng_msg ��Դ��ȷ���Զ��ͷ�
    // - ֧���ƶ�������ƶ���ֵ�����ÿ����Ա�֤��Դ��ռ
    // - �ṩ����Ϣͷ�������ĵ�׷�ӡ����롢�ü��Ȳ���
    // - ֧���޷����������ַ����ı�ݲ���
    // - �쳣��ȫ������ʧ�ܻ��������ʱ�׳� Exception
    class Msg
    {
    public:
        // ���캯����ʹ�����е� nng_msg ָ���ʼ����Ϣ����
        // ������msg - nng_msg ָ�룬Ĭ��Ϊ nullptr
        explicit Msg(nng_msg* msg = nullptr) noexcept : _My_msg(msg) {
        }

        // ���캯��������ָ����ʼ��С�Ŀ���Ϣ
        // ������size - ��Ϣ�ĳ�ʼ��С
        // �쳣��������ʧ�ܣ��׳� Exception
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

        // ���캯����ʹ��ָ�����ݺʹ�С������Ϣ
        // ������data - ����ָ�룬data_size - ���ݴ�С
        // �쳣��������ʧ�ܣ��׳� Exception
        explicit Msg(const void* data, size_t data_size) noexcept(false) {
            int rv = nng_msg_alloc(&_My_msg, data_size);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_alloc");
            }
            std::memcpy(body(), data, data_size);
        }

        // ���캯����ʹ�� nng_iov �ṹ������Ϣ
        // ������iov - �������ݻ������ͳ��ȵ� nng_iov �ṹ
        // �쳣��������ʧ�ܣ��׳� Exception
        explicit Msg(const nng_iov& iov) noexcept(false) {
            int rv = nng_msg_alloc(&_My_msg, iov.iov_len);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_alloc");
            }
            std::memcpy(body(), iov.iov_buf, iov.iov_len);
        }

        // �����������ͷ���Ϣ��Դ
        ~Msg() noexcept {
            if (_My_msg) {
                nng_msg_free(_My_msg);
            }
        }

        // �ƶ����캯����ת����Ϣ����Ȩ
        // ������other - Դ��Ϣ����
        Msg(Msg&& other) noexcept : _My_msg(other._My_msg) {
            other._My_msg = nullptr;
        }

        // �ƶ���ֵ�������ת����Ϣ����Ȩ
        // ������other - Դ��Ϣ����
        // ���أ���ǰ���������
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

        // ���ÿ������캯��
        Msg(const Msg&) = delete;

        // ���ÿ�����ֵ�����
        Msg& operator=(const Msg&) = delete;

        // ���·�����Ϣ��С
        // ������size - �µ���Ϣ��С
        // ���أ����������0 ��ʾ�ɹ�
        // �쳣�������׳�����ʧ�ܵ��쳣
        int realloc(size_t size) noexcept {
            if (_My_msg == nullptr) {
                return nng_msg_alloc(&_My_msg, size);
            }
            else {
                return nng_msg_realloc(_My_msg, size);
            }
        }

        // Ԥ����Ϣ�ռ�
        // ������size - Ԥ���Ŀռ��С
        // ���أ����������0 ��ʾ�ɹ�
        int reserve(size_t size) noexcept {
            return nng_msg_reserve(_My_msg, size);
        }

        // ��ȡ��Ϣ��ǰ����
        // ���أ���Ϣ��������С
        size_t capacity() const noexcept {
            return nng_msg_capacity(_My_msg);
        }

        // ��ȡ��Ϣͷ��ָ��
        // ���أ�ָ����Ϣͷ����ָ��
        void* header() const noexcept {
            return nng_msg_header(_My_msg);
        }

        // ��ȡ��Ϣͷ������
        // ���أ���Ϣͷ���ĳ���
        size_t header_len() const noexcept {
            return nng_msg_header_len(_My_msg);
        }

        // ��ȡ��Ϣ����ָ��
        // ���أ�ָ����Ϣ���ĵ�ָ��
        void* body() const noexcept {
            return nng_msg_body(_My_msg);
        }

        // ��ȡ��Ϣ���ĳ���
        // ���أ���Ϣ���ĵĳ���
        size_t len() const noexcept {
            return nng_msg_len(_My_msg);
        }

        // ��ȡ������Ϣ����β�� offset �ֽڴ�������ָ��
        // ��;����ݷ�����Ϣ����ĩβ���������ݣ���Э��β���ֶεȣ�
        // ������offset - ��������β�����ֽ�ƫ�ƣ�0 ��ʾ���һ���ֽڣ�
        // ���أ�ָ���ƫ�ƴ���ָ�룬�� offset �������ĳ����򷵻� nullptr
        void* body_tail(size_t offset) const noexcept {
            size_t l = len();
            if (offset > l) return nullptr;
            return static_cast<uint8_t*>(body()) + (l - offset);
        }

        // ����Ϣ����׷������
        // ������data - ����ָ�룬data_size - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int append(const void* data, size_t data_size) noexcept {
            return nng_msg_append(_My_msg, data, data_size);
        }

        // ����Ϣ���Ŀ�ͷ��������
        // ������data - ����ָ�룬data_size - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int insert(const void* data, size_t data_size) noexcept {
            return nng_msg_insert(_My_msg, data, data_size);
        }

        // ����Ϣ���Ŀ�ͷ�ü�ָ����С������
        // ������size - Ҫ�ü������ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int trim(size_t size) noexcept {
            return nng_msg_trim(_My_msg, size);
        }

        // ����Ϣ����ĩβ�ü�ָ����С������
        // ������size - Ҫ�ü������ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int chop(size_t size) noexcept {
            return nng_msg_chop(_My_msg, size);
        }

        // ����Ϣͷ��׷������
        // ������data - ����ָ�룬data_size - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int header_append(const void* data, size_t data_size) noexcept {
            return nng_msg_header_append(_My_msg, data, data_size);
        }

        // ����Ϣͷ����ͷ��������
        // ������data - ����ָ�룬data_size - ���ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int header_insert(const void* data, size_t data_size) noexcept {
            return nng_msg_header_insert(_My_msg, data, data_size);
        }

        // ����Ϣͷ����ͷ�ü�ָ����С������
        // ������size - Ҫ�ü������ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int header_trim(size_t size) noexcept {
            return nng_msg_header_trim(_My_msg, size);
        }

        // ����Ϣͷ��ĩβ�ü�ָ����С������
        // ������size - Ҫ�ü������ݴ�С
        // ���أ����������0 ��ʾ�ɹ�
        int header_chop(size_t size) noexcept {
            return nng_msg_header_chop(_My_msg, size);
        }

        // ����Ϣͷ��׷�� 16 λ�޷�������
        // ������val - Ҫ׷�ӵ� 16 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int header_append_u16(uint16_t val) noexcept {
            return nng_msg_header_append_u16(_My_msg, val);
        }

        // ����Ϣͷ��׷�� 32 λ�޷�������
        // ������val - Ҫ׷�ӵ� 32 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int header_append_u32(uint32_t val) noexcept {
            return nng_msg_header_append_u32(_My_msg, val);
        }

        // ����Ϣͷ��׷�� 64 λ�޷�������
        // ������val - Ҫ׷�ӵ� 64 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int header_append_u64(uint64_t val) noexcept {
            return nng_msg_header_append_u64(_My_msg, val);
        }

        // ����Ϣͷ����ͷ���� 16 λ�޷�������
        // ������val - Ҫ����� 16 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int header_insert_u16(uint16_t val) noexcept {
            return nng_msg_header_insert_u16(_My_msg, val);
        }

        // ����Ϣͷ����ͷ���� 32 λ�޷�������
        // ������val - Ҫ����� 32 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int header_insert_u32(uint32_t val) noexcept {
            return nng_msg_header_insert_u32(_My_msg, val);
        }

        // ����Ϣͷ����ͷ���� 64 λ�޷�������
        // ������val - Ҫ����� 64 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int header_insert_u64(uint64_t val) noexcept {
            return nng_msg_header_insert_u64(_My_msg, val);
        }

        // ����Ϣͷ��ĩβ�ü� 16 λ�޷�������
        // ������val - �洢�ü����� 16 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int header_chop_u16(uint16_t* val) noexcept {
            return nng_msg_header_chop_u16(_My_msg, val);
        }

        // ����Ϣͷ��ĩβ�ü� 16 λ�޷�������
        // ���أ��ü����� 16 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint16_t header_chop_u16() noexcept(false) {
            uint16_t v;
            int rv = nng_msg_header_chop_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_chop_u16");
            }
            return v;
        }

        // ����Ϣͷ��ĩβ�ü� 32 λ�޷�������
        // ������val - �洢�ü����� 32 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int header_chop_u32(uint32_t* val) noexcept {
            return nng_msg_header_chop_u32(_My_msg, val);
        }

        // ����Ϣͷ��ĩβ�ü� 32 λ�޷�������
        // ���أ��ü����� 32 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint32_t header_chop_u32() noexcept(false) {
            uint32_t v;
            int rv = nng_msg_header_chop_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_chop_u32");
            }
            return v;
        }

        // ����Ϣͷ��ĩβ�ü� 64 λ�޷�������
        // ������val - �洢�ü����� 64 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int header_chop_u64(uint64_t* val) noexcept {
            return nng_msg_header_chop_u64(_My_msg, val);
        }

        // ����Ϣͷ��ĩβ�ü� 64 λ�޷�������
        // ���أ��ü����� 64 λ�޷����������� optional ��ʽ��
        // �쳣��������ʧ�ܣ��׳� Exception
        std::optional<uint64_t> header_chop_u64() noexcept(false) {
            uint64_t v;
            int rv = nng_msg_header_chop_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_chop_u64");
            }
            return v;
        }

        // ����Ϣͷ����ͷ�ü� 16 λ�޷�������
        // ������val - �洢�ü����� 16 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int header_trim_u16(uint16_t* val) noexcept {
            return nng_msg_header_trim_u16(_My_msg, val);
        }

        // ����Ϣͷ����ͷ�ü� 16 λ�޷�������
        // ���أ��ü����� 16 λ�޷����������� optional ��ʽ��
        // �쳣��������ʧ�ܣ��׳� Exception
        std::optional<uint16_t> header_trim_u16() noexcept(false) {
            uint16_t v;
            int rv = nng_msg_header_trim_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_trim_u16");
            }
            return v;
        }

        // ����Ϣͷ����ͷ�ü� 32 λ�޷�������
        // ������val - �洢�ü����� 32 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int header_trim_u32(uint32_t* val) noexcept {
            return nng_msg_header_trim_u32(_My_msg, val);
        }

        // ����Ϣͷ����ͷ�ü� 32 λ�޷�������
        // ���أ��ü����� 32 λ�޷����������� optional ��ʽ��
        // �쳣��������ʧ�ܣ��׳� Exception
        std::optional<uint32_t> header_trim_u32() noexcept(false) {
            uint32_t v;
            int rv = nng_msg_header_trim_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_trim_u32");
            }
            return v;
        }

        // ����Ϣͷ����ͷ�ü� 64 λ�޷�������
        // ������val - �洢�ü����� 64 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int header_trim_u64(uint64_t* val) noexcept {
            return nng_msg_header_trim_u64(_My_msg, val);
        }

        // ����Ϣͷ����ͷ�ü� 64 λ�޷�������
        // ���أ��ü����� 64 λ�޷����������� optional ��ʽ��
        // �쳣��������ʧ�ܣ��׳� Exception
        std::optional<uint64_t> header_trim_u64() noexcept(false) {
            uint64_t v;
            int rv = nng_msg_header_trim_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_header_trim_u64");
            }
            return v;
        }

        // ����Ϣ����׷�� 16 λ�޷�������
        // ������val - Ҫ׷�ӵ� 16 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int append_u16(uint16_t val) noexcept {
            return nng_msg_append_u16(_My_msg, val);
        }

        // ����Ϣ����׷�� 32 λ�޷�������
        // ������val - Ҫ׷�ӵ� 32 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int append_u32(uint32_t val) noexcept {
            return nng_msg_append_u32(_My_msg, val);
        }

        // ����Ϣ����׷�� 64 λ�޷�������
        // ������val - Ҫ׷�ӵ� 64 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int append_u64(uint64_t val) noexcept {
            return nng_msg_append_u64(_My_msg, val);
        }

        // ����Ϣ����׷���ַ���
        // ������sv - Ҫ׷�ӵ��ַ�����ͼ
        // ���أ����������0 ��ʾ�ɹ�
        int append_string(std::string_view sv) noexcept {
            int rv = nng_msg_append(_My_msg, sv.data(), sv.size());
            if (rv != NNG_OK) {
                return rv;
            }
            return nng_msg_append_u32(_My_msg, (uint32_t)sv.size());
        }

        // ����Ϣ���Ŀ�ͷ���� 16 λ�޷�������
        // ������val - Ҫ����� 16 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int insert_u16(uint16_t val) noexcept {
            return nng_msg_insert_u16(_My_msg, val);
        }

        // ����Ϣ���Ŀ�ͷ���� 32 λ�޷�������
        // ������val - Ҫ����� 32 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int insert_u32(uint32_t val) noexcept {
            return nng_msg_insert_u32(_My_msg, val);
        }

        // ����Ϣ���Ŀ�ͷ���� 64 λ�޷�������
        // ������val - Ҫ����� 64 λ�޷�������
        // ���أ����������0 ��ʾ�ɹ�
        int insert_u64(uint64_t val) noexcept {
            return nng_msg_insert_u64(_My_msg, val);
        }

        // ����Ϣ���Ŀ�ͷ�����ַ���
        // ������sv - Ҫ������ַ�����ͼ
        // ���أ����������0 ��ʾ�ɹ�
        int insert_string(std::string_view sv) noexcept {
            int rv = nng_msg_insert(_My_msg, sv.data(), sv.size());
            if (rv != NNG_OK) {
                return rv;
            }
            return nng_msg_insert_u32(_My_msg, (uint32_t)sv.size());
        }

        // ����Ϣ����ĩβ�ü� 16 λ�޷�������
        // ������val - �洢�ü����� 16 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int chop_u16(uint16_t* val) noexcept {
            return nng_msg_chop_u16(_My_msg, val);
        }

        // ����Ϣ����ĩβ�ü� 16 λ�޷�������
        // ���أ��ü����� 16 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint16_t chop_u16() noexcept(false) {
            uint16_t v;
            auto rv = nng_msg_chop_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_chop_u16");
            }
            return v;
        }

        // ����Ϣ����ĩβ�ü� 32 λ�޷�������
        // ������val - �洢�ü����� 32 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int chop_u32(uint32_t* val) noexcept {
            return nng_msg_chop_u32(_My_msg, val);
        }

        // ����Ϣ����ĩβ�ü� 32 λ�޷�������
        // ���أ��ü����� 32 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint32_t chop_u32() noexcept(false) {
            uint32_t v;
            auto rv = nng_msg_chop_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_chop_u32");
            }
            return v;
        }

        // ����Ϣ����ĩβ�ü� 64 λ�޷�������
        // ������val - �洢�ü����� 64 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int chop_u64(uint64_t* val) noexcept {
            return nng_msg_chop_u64(_My_msg, val);
        }

        // ����Ϣ����ĩβ�ü� 64 λ�޷�������
        // ���أ��ü����� 64 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint64_t chop_u64() noexcept(false) {
            uint64_t v;
            auto rv = nng_msg_chop_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_chop_u64");
            }
            return v;
        }

        // ����Ϣ����ĩβ�ü��ַ���
        // ������s - �洢�ü������ַ���
        // ���أ����������0 ��ʾ�ɹ�
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

        // ����Ϣ����ĩβ�ü��ַ���
        // ���أ��ü������ַ���
        // �쳣��������ʧ�ܣ��׳� Exception
        std::string chop_string() noexcept(false) {
            std::string s;
            auto rv = chop_string(s);
            if (rv != NNG_OK) {
                throw Exception(rv, "chop_string");
            }
            return s;
        }

        // ����Ϣ���Ŀ�ͷ�ü� 16 λ�޷�������
        // ������val - �洢�ü����� 16 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int trim_u16(uint16_t* val) noexcept {
            return nng_msg_trim_u16(_My_msg, val);
        }

        // ����Ϣ���Ŀ�ͷ�ü� 16 λ�޷�������
        // ���أ��ü����� 16 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint16_t trim_u16() noexcept(false) {
            uint16_t v;
            auto rv = nng_msg_trim_u16(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_trim_u16");
            }
            return v;
        }

        // ����Ϣ���Ŀ�ͷ�ü� 32 λ�޷�������
        // ������val - �洢�ü����� 32 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int trim_u32(uint32_t* val) noexcept {
            return nng_msg_trim_u32(_My_msg, val);
        }

        // ����Ϣ���Ŀ�ͷ�ü� 32 λ�޷�������
        // ���أ��ü����� 32 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint32_t trim_u32() noexcept(false) {
            uint32_t v;
            auto rv = nng_msg_trim_u32(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_trim_u32");
            }
            return v;
        }

        // ����Ϣ���Ŀ�ͷ�ü� 64 λ�޷�������
        // ������val - �洢�ü����� 64 λ�޷���������ָ��
        // ���أ����������0 ��ʾ�ɹ�
        int trim_u64(uint64_t* val) noexcept {
            return nng_msg_trim_u64(_My_msg, val);
        }

        // ����Ϣ���Ŀ�ͷ�ü� 64 λ�޷�������
        // ���أ��ü����� 64 λ�޷�������
        // �쳣��������ʧ�ܣ��׳� Exception
        uint64_t trim_u64() noexcept(false) {
            uint64_t v;
            auto rv = nng_msg_trim_u64(_My_msg, &v);
            if (rv != NNG_OK) {
                throw Exception(rv, "nng_msg_trim_u64");
            }
            return v;
        }

        // ����Ϣ���Ŀ�ͷ�ü��ַ���
        // ������s - �洢�ü������ַ���
        // ���أ����������0 ��ʾ�ɹ�
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

        // ����Ϣ���Ŀ�ͷ�ü��ַ���
        // ���أ��ü������ַ���
        // �쳣��������ʧ�ܣ��׳� Exception
        std::string trim_string() noexcept(false) {
            std::string s;
            int rv = trim_string(s);
            if (rv != NNG_OK) {
                throw Exception(rv, "trim_string");
            }
            return s;
        }

        // ������Ϣ��Ŀ����Ϣ����
        // ������dest - Ŀ����Ϣ����
        // ���أ����������0 ��ʾ�ɹ�
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

        // �����Ϣ����
        void clear() noexcept {
            nng_msg_clear(_My_msg);
        }

        // �����Ϣͷ��
        void header_clear() noexcept {
            nng_msg_header_clear(_My_msg);
        }

        // ���ù����Ĺܵ�
        // ������pipe - Ҫ���õĹܵ�
        void set_pipe(nng_pipe pipe) noexcept {
            nng_msg_set_pipe(_My_msg, pipe);
        }

        // ��ȡ�����Ĺܵ�
        // ���أ������Ĺܵ�
        nng_pipe get_pipe() const noexcept {
            return nng_msg_get_pipe(_My_msg);
        }

        // ��ֵ������������µ� nng_msg ָ��
        // ������msg - �µ� nng_msg ָ��
        // ���أ���ǰ���������
        Msg& operator =(nng_msg* msg) noexcept {
            if (_My_msg != msg) {
                if (_My_msg) {
                    nng_msg_free(_My_msg);
                }
                _My_msg = msg;
            }
            return *this;
        }

        // �ͷ���Ϣ����Ȩ
        // ���أ���ǰ����� nng_msg ָ��
        nng_msg* release() noexcept {
            return std::exchange(_My_msg, nullptr);
        }

        // �����Ϣ�Ƿ���Ч
        // ���أ�true ��ʾ��Ϣ��Ч��false ��ʾ��Ч
        bool valid() const noexcept {
            return _My_msg != nullptr;
        }

        // ת��Ϊ nng_msg ָ��
        // ���أ���ǰ����� nng_msg ָ��
        operator nng_msg* () const noexcept { return _My_msg; }

        // �����Ϣ�Ƿ���Ч������ת����
        // ���أ�true ��ʾ��Ϣ��Ч��false ��ʾ��Ч
        operator bool() const noexcept { return valid(); }

    public:
        typedef uint64_t _Ty_msg_code;
        typedef _Ty_msg_code _Ty_msg_result;

        // ����Ϣ�ü���Ϣ����
        // ������m - ��Ϣ����
        // ���أ��ü�������Ϣ����
        // �쳣��������ʧ�ܣ��׳� Exception
        static inline _Ty_msg_code _Chop_msg_code(Msg& m) noexcept(false) {
            return m.chop_u64();
        }

        // ����Ϣ׷����Ϣ����
        // ������m - ��Ϣ����code - Ҫ׷�ӵ���Ϣ����
        static inline void _Append_msg_code(Msg& m, _Ty_msg_code code) noexcept {
            m.append_u64(code);
        }

        // ����Ϣ�ü���Ϣ���
        // ������m - ��Ϣ����
        // ���أ��ü�������Ϣ���
        // �쳣��������ʧ�ܣ��׳� Exception
        static inline _Ty_msg_result _Chop_msg_result(Msg& m) noexcept(false) {
            return m.chop_u64();
        }

        // ����Ϣ׷����Ϣ���
        // ������m - ��Ϣ����result - Ҫ׷�ӵ���Ϣ���
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