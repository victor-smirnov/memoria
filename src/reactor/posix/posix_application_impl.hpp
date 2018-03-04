
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

#include <stdlib.h>
#include <unistd.h>

#ifdef MMA1_MACOSX
extern char **environ;
#endif

namespace memoria {
namespace v1 {
namespace reactor {

class EnvironmentImpl {

public:
	EnvironmentImpl(const char* const* envp) {}

	Optional<U16String> get(const U16String& name) 
	{
        auto value_ptr = ::getenv(name.to_u8().data());

        if (value_ptr) {
            return U16String(value_ptr);
        }
        else {
            return Optional<U16String>();
        }
	}


	void set(const U16String& name, const U16String& value) 
	{
        if (::setenv(name.to_u8().data(), value.to_u8().data(), 1) < 0)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't set environment variable: {}={}", name, value);
        }
	}

	EnvironmentMap entries() 
	{
		EnvironmentMap map;

		for (auto& entry: entries_list()) 
		{
			size_t idx = entry.find_first_of(u"=");
			if (idx != U16String::NPOS)
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
        EnvironmentList list;
        auto envp = environ;

        while(envp && *envp)
        {
            list.emplace_back(*envp);
            envp++;
        }

        return list;
	}
};


}}}
