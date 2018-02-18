
// Copyright 2018 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/config.hpp>
           
#if defined(_MSC_VER)

#ifndef MMA1_CLANG

namespace memoria {
namespace v1 {

uint32_t __inline __builtin_ctz(uint32_t value)
{
	DWORD trailing_zero = 0;

	if (_BitScanForward(&trailing_zero, value))
	{
		return trailing_zero;
	}
	else
	{
		return 32;
	}
	return 0;
}

uint32_t __inline __builtin_ctzll(uint64_t value)
{
	DWORD trailing_zero = 0;

	if (_BitScanForward64(&trailing_zero, value))
	{
		return trailing_zero;
	}
	else
	{
		return 64;
	}
	return 0;
}

uint32_t __inline __builtin_clz(uint32_t value)
{
	DWORD leading_zero = 0;

	if (_BitScanReverse(&leading_zero, value))
	{
		return 31 - leading_zero;
	}
	else
	{
		return 32;
	}
	return 0;
}

uint32_t __inline __builtin_clzll(uint64_t value)
{
	DWORD leading_zero = 0;

	if (_BitScanReverse64(&leading_zero, value))
	{
		return 63 - leading_zero;
	}
	else
	{
		return 64;
	}
	return 0;
}

}}

#endif
#endif