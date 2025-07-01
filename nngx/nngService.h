#pragma once

#include "nngException.h"

namespace nng
{
	// Service �ࣺNNG ����ģ���࣬�̳���ָ���Ļ���
	// ��;���ṩ�� NNG ������첽���ȹ���֧��������ֹͣ�����߳�
	// ���ԣ�
	// - ����ģ����� _TyBase ��չ���ܣ������ڲ�ͬ���͵� NNG ���������� Listener �� Dialer��
	// - ʹ�� RAII ��������̣߳�ȷ��������ʱ�Զ�ֹͣ
	// - ֧���첽���ȣ�ͨ�������̵߳��û���� dispatch ����
	template <typename _TyBase>
	class Service
		: public _TyBase
	{
	public:
		// ����������ֹͣ�����̲߳�������Դ
		virtual ~Service()
		{
			stop_dispatch();
		}
	public:
		// ���������߳�
		// ������addr - ������������ӵĵ�ַ��flags - ������־��Ĭ��Ϊ 0
		// ���أ����������0 ��ʾ�ɹ����� 0 ��ʾʧ��
		// �쳣������������ʧ�ܣ������׳� Exception
		int start_dispatch(std::string_view addr, int flags = 0) noexcept(false)
		{
			int rv = _TyBase::start(addr, flags);
			if (rv != NNG_OK) {
				return rv;
			}

			_My_dispatch_thread = std::thread(
				[=]
				{
					_TyBase::dispatch();
				}
			);

			return NNG_OK;
		}

		// �������߳��Ƿ�ɼ���
		// ���أ�true ��ʾ�߳̿ɼ��룬false ��ʾ���ɼ���
		inline bool joinable() noexcept
		{
			return _My_dispatch_thread.joinable();
		}

		// ��������̣߳��ȴ�����ɡ�
		// ע�⣺�����߳̿ɼ���ʱ���á�
		inline void join() {
			_My_dispatch_thread.join();
		}

		// ֹͣ�����߳�
		// ���أ�true ��ʾ�ɹ�ֹͣ�������̣߳�false ��ʾ�̲߳��ɼ���
		// ˵�������û���� close �����ر������������ȴ������߳̽���
		bool stop_dispatch() noexcept
		{
			if (!_My_dispatch_thread.joinable()) {
				return false;
			}

			_TyBase::close();

			_My_dispatch_thread.join();

			return true;
		}
	protected:
		std::thread _My_dispatch_thread;    // _Tcp_base_parser -> virtual class
	};
}