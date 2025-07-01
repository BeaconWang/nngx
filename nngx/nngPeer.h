#pragma once

#include <memory>
#include <string_view>

#include "nngException.h"
#include "nngSocket.h"
#include "nngListener.h"
#include "nngDialer.h"

namespace nng
{
    // Peer �ࣺNNG �׽��ֵ�ģ���࣬�̳� Socket��֧�� Listener �� Dialer ������
    // ��;���ṩ�׽��ֵĴ�������������������������֧�� Listener �� Dialer ���͵�����
    // ���ԣ�
    // - ʹ�� RAII ͨ�� std::unique_ptr ������������Դ
    // - ֧��ģ�廯���������ͣ�Listener �� Dialer����ͨ�� static_assert ����
    // - �ṩ�麯���ӿ���֧��������չ
    // - �쳣��ȫ������������ʧ��ʱ�����׳� Exception
    template<class _Connector_t = Listener>
    class Peer : virtual public Socket
    {
        static_assert(
            std::is_same_v<_Connector_t, Listener> || std::is_same_v<_Connector_t, Dialer>,
            "_Connector_t must be either Listener or Dialer"
            );

        // �����׽��ֵ��麯����������ʵ��
        // ���أ����������0 ��ʾ�ɹ�
        virtual int _Create() noexcept = 0;

    public:
        // ��������
        // ������addr - ���ӵ�ַ��flags - ������־��Ĭ��Ϊ 0
        // ���أ����������0 ��ʾ�ɹ�
        // �쳣���������׽��ֻ�������ʧ�ܣ��׳� Exception
        int start(std::string_view addr, int flags = 0) noexcept(false) {
            int rv = _Create();
            if (rv != NNG_OK) {
                return rv;
            }

            _My_connector = std::make_unique<_Connector_t>(*this, addr);
            return _My_connector->start(flags);
        }

        // ��ȡ������ָ��
        // ���أ�ָ����������ԭʼָ��
        _Connector_t* get_connector() const noexcept { return _My_connector.get(); }

    protected:
        std::unique_ptr<_Connector_t> _My_connector;
    };
}