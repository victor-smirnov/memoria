
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

#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/core/strings/string.hpp>

#include <boost/winapi/environment.hpp>


namespace memoria {
namespace v1 {
namespace reactor {

class EnvironmentImpl {

public:
	EnvironmentImpl(const char* const* envp) {}

	Optional<U8String> get(const U8String& name) 
	{
		UWString wname = name.to_uwstring();

		DWORD bufsize = boost::winapi::GetEnvironmentVariableW(wname.data(), nullptr, 0);

		if (bufsize > 0) 
		{
			if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
				return Optional<U8String>();
			}
			else {
				auto buf = allocate_system_zeroed<wchar_t>(bufsize * sizeof(wchar_t));
				
				DWORD bufsize2 = boost::winapi::GetEnvironmentVariableW(wname.data(), buf.get(), bufsize);

				if (bufsize2 <= bufsize) 
				{
					return UWString(buf.get(), bufsize2).to_u16();
				}
				else {
                    MMA1_THROW(SystemException()) << format_ex("Incorrect estimation of env variable's '{}' value size {}:{}", name, bufsize, bufsize2);
				}
			}
		}
		else {
            MMA1_THROW(SystemException()) << format_ex("Can't obtain value of environment variable {}", name);
		}
	}


	void set(const U8String& name, const U8String& value) 
	{
		if (!boost::winapi::SetEnvironmentVariableW(name.to_uwstring().data(), value.to_uwstring().data())) 
		{
            MMA1_THROW(SystemException()) << format_ex("Can't set value for environment variable: {}={}", name, value);
		}
	}

	EnvironmentMap entries() 
	{
		EnvironmentMap map;

		for (auto& entry: entries_list()) 
		{
			size_t idx = entry.find_first_of("=");
			if (idx != U8String::NPOS)
			{
				auto key = entry.substring(0, idx);
				auto value = entry.substring(idx + 1);
				map[key] = value;
			}
		}

		return map;
	}

	EnvironmentList entries_list() 
	{
		auto env_list_ptr = MakeScopedDtr(boost::winapi::GetEnvironmentStringsW(), [](auto* list_ptr) {
			if (list_ptr != nullptr) {
				boost::winapi::FreeEnvironmentStringsW(list_ptr);
			}
		});

		if (env_list_ptr) 
		{
			EnvironmentList list;

			size_t start{};
			size_t idx{};
			bool end_entry{true};

			while (true) 
			{
				if (*(env_list_ptr.get() + idx) == 0) 
				{
					if (!end_entry) 
					{
						list.emplace_back(UWString(env_list_ptr.get() + start, idx - start).to_u16());
						start = idx + 1;
						end_entry = true;
					}
					else {
						break;
					}
				}
				else {
					end_entry = false;
				}

				idx++;
			}

			return list;
		}
		else {
			MMA1_THROW(SystemException()) << WhatCInfo("Can't obtain environment variables");
		}
	}
};


}}}
