#pragma once

#include "nngException.h"
#include "nngListener.h"

namespace nng::util
{
	// 初始化 NNG 库
	// 返回：操作结果，0 表示成功
	int initialize() noexcept;

	// 释放 NNG 库资源
	void uninitialize() noexcept;

    std::string _Pre_address(std::string_view _Address) noexcept;

    void _Pre_start_listen(nng::Listener& _Connector_ref) noexcept;
}