
// Copyright 2019-2022 Victor Smirnov
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

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/reflection/typehash.hpp>

#include <memoria/api/common/ctr_api_btss.hpp>

#include <memoria/api/common/ctr_input_btss.hpp>

namespace memoria {

class AllocationMap {};

template <typename Profile>
struct ICtrApiTypes<AllocationMap, Profile> {
    using CtrInputBuffer = detail::BIStreamSize<size_t>;
};


template <>
struct TypeHash<AllocationMap>: UInt64Value<
    5663493242560930
> {};

template <>
struct DataTypeTraits<AllocationMap> {

    using Parameters = TL<>;

    static constexpr bool HasTypeConstructors = false;
    static constexpr bool isSdnDeserializable = false;
};

}
