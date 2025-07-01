#pragma once

#include "nngException.h"
#include "nngListener.h"

namespace nng::util
{
	// ��ʼ�� NNG ��
	// ���أ����������0 ��ʾ�ɹ�
	int initialize() noexcept;

	// �ͷ� NNG ����Դ
	void uninitialize() noexcept;

    std::string _Pre_address(std::string_view _Address) noexcept;

    void _Pre_start_listen(nng::Listener& _Connector_ref) noexcept;
}