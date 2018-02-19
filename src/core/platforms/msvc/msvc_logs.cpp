
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/container/logs.hpp>

namespace memoria {
namespace v1 {

const char* ExtractFunctionName(const char* full_name)
{
    const char* start = NULL;

    for (const char* tmp = full_name; *tmp != 0; tmp++)
    {
        if (*tmp == ':')
        {
            start = tmp;
        }
    }

    if (start == NULL) {
        start = full_name;
    }
    else {
        start++;
    }

    return start;
}

}}
