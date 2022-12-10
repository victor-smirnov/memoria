
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

#include <memoria/core/types.hpp>

#include <absl/types/span.h>

namespace memoria {


template <typename T>
using VarbinaryView = absl::Span<T>;

template <>
struct DataTypeTraits<Varbinary>: DataTypeTraitsBase<Varbinary>
{
    using AtomType      = uint8_t;
    using ViewType      = VarbinaryView<AtomType>;
    using ConstViewType = VarbinaryView<const AtomType>;

    static constexpr bool isDataType          = true;
    static constexpr bool HasTypeConstructors = false;

    using SharedPtrT = Own<ViewType>;
    using ConstSharedPtrT = DTConstSharedPtr<ViewType>;

    using SpanT = DTViewSpan<ViewType, SharedPtrT>;
    using ConstSpanT = DTConstViewSpan<ViewType, ConstSharedPtrT>;

    static void create_signature(SBuf& buf, const Varchar& obj)
    {
        buf << "Varbinary";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Varbinary";
    }
};


}
