
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/linked/document/linked_document.hpp>

namespace memoria {

enum class CheckSeverity {
    ERROR
};

using CheckResultConsumerFn = std::function<void (CheckSeverity, const LDDocument&)>;


template <typename... Args>
LDDocument make_string_document(const char* fmt, Args&&... args)
{
    LDDocument doc;
    doc.set_varchar(format_u8(fmt, std::forward<Args>(args)...));
    return doc;
}

}
