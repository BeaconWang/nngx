#pragma once

#include "nngException.h"

namespace nng
{
	// Hooker �ࣺΪ NNG �������ṩ���ӹ��ܵ�ģ����
	// ��;���������ض����������ַ�����������������ǰִ���û�����Ļص�����
	// ���ԣ�
	// - �ṩ��̬�������õ�ַ���������ǰ�Ļص�����
	// - ʹ�û�����ȷ���̰߳�ȫ�Ļص����ú͵���
	// - ֧��ģ������������ڲ�ͬ���͵����������� Listener �� Dialer��
	template < class _TyConnector>
	class Hooker
	{
	public:
		// ����ص��������ͣ�����Ԥ�����ַ
		using _Pre_address_t = std::function<std::string(std::string_view sv)>;
		// ����ص��������ͣ�����������������ǰִ��
		using _Pre_start_t = std::function<void(_TyConnector& _Connector_ref)>;
	public:
		// ���õ�ַԤ����ص�����
		// ������_Callback - �û�����ĵ�ַ����ص�
		// ˵�����̰߳�ȫ��ͨ�������������ص�����
		static void set_pre_address(_Pre_address_t _Callback) noexcept {
			std::lock_guard<std::mutex> locker(_My_mtx);
			_My_pre_address = _Callback;
		}

		// ��������ǰԤ����ص�����
		// ������_Callback - �û����������ǰ�ص�
		// ˵�����̰߳�ȫ��ͨ�������������ص�����
		static void set_pre_start(_Pre_start_t _Callback) noexcept {
			std::lock_guard<std::mutex> locker(_My_mtx);
			_My_pre_start = _Callback;
		}
	protected:
		// Ԥ�����ַ
		// ������_Address - ����ĵ�ַ�ַ�����ͼ
		// ���أ������ĵ�ַ�ַ���
		// ˵������������˻ص�������ûص������ַ�����򷵻�ԭʼ��ַ
		static std::string _Pre_address(std::string_view _Address) noexcept {
			std::lock_guard<std::mutex> locker(_My_mtx);
			if (_My_pre_address) {
				return _My_pre_address(_Address);
			}

			return _Address.data();
		}

		// ������������ǰִ��Ԥ����
		// ������_Connector_ref - ���������������
		// ˵������������˻ص�������ûص�����Ԥ����
		static void _Pre_start(_TyConnector& _Connector_ref) noexcept {
			std::lock_guard<std::mutex> locker(_My_mtx);
			if (_My_pre_start) {
				_My_pre_start(_Connector_ref);
			}
		}
	private:
		// ��̬�������������ص����������ú͵���
		inline static std::mutex _My_mtx;
		// ��̬��ַԤ����ص�
		inline static _Pre_address_t _My_pre_address;
		// ��̬����ǰԤ����ص�
		inline static _Pre_start_t _My_pre_start;
	};
}