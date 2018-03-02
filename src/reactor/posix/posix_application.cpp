
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

namespace _ {

U16String get_image_name(const std::vector<U16String>& args) 
{
	if (args.size() > 0) 
	{
		auto& arg0 = args[0];

		size_t start = arg0.find_last_of(u"/");

		if (start == U16String::NPOS) {
			start = 0;
		}

		return arg0.substring(start);
	}
	else {
		return "none";
	}
}

}

}}}