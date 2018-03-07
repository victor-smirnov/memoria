
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/reactor/application.hpp>

namespace memoria {
namespace v1 {
namespace reactor {



namespace {

auto find_last_of(const U16String& str, const char16_t* pp) 
{
	size_t pos0 = U16String::NPOS;
	size_t pos  = 0;
	while (pos = str.find_first_of(pp, pos)) 
	{
		if (pos != U16String::NPOS) 
		{
			pos0 = pos;
			pos++;
		}
		else {
			break;
		}
	}

	return pos0;
}

}


filesystem::path get_image_name()
{
    auto name = get_program_path().filename().to_u16();
    
    size_t pos = find_last_of(name, u".");
    
    if (pos != U16String::NPOS) {
	return name.substring(0, pos);
    }
    else {
	return name;
    }
}



}}}