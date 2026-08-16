// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <arch/x86_64/std/stdint.hpp>

// API Definitions

enum API_STATUS : uint32_t
{
	API_SUCCESS = 0x00,
	API_INVALID_PARAMETER = 0x01,
	API_UNSUPPORTED = 0x02,
	API_NOT_FOUND = 0x03,
	API_MEMORY_ALLOC_FAILED = 0x04,

	API_RMGR_INVALID_TREE_STRUCTURE = 0x10
};

#define API_ERROR(status) (status != API_SUCCESS)