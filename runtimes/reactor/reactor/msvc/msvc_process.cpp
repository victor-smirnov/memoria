
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


#include "msvc_process_impl.hpp"

#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

#include <boost/winapi/dll.hpp>

namespace memoria {
namespace reactor {



filesystem::path get_program_path()
{
	DWORD bSize = 65535;
	auto buf = allocate_system_zeroed<wchar_t>(bSize + 1);

	DWORD actual_size = boost::winapi::GetModuleFileNameW(nullptr, buf.get(), bSize + 1);
	if (GetLastError() == 0) 
	{
		return filesystem::path(UWString(buf.get(), actual_size).to_std_string());
	}
	else {
		MMA_THROW(SystemException()) << WhatCInfo("Can't obtain this module's file name");
	}
}


}}
