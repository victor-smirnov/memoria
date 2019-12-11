
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <absl/types/span.h>
#include <ostream>


namespace memoria {
namespace v1 {

template <typename T>
using Span = absl::Span<T>;

template <typename T>
std::ostream& operator<<(std::ostream& out, Span<const T> span)
{
    bool first_iter = true;

    out << "[";

    for (auto& vv: span)
    {
        if (!first_iter)
        {
            out << ", ";
        }
        else {
            first_iter = false;
        }

        out << vv;
    }

    out << "]";

    return out;
}

}}
