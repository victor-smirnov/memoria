
// Copyright 2022 Victor Smirnov
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
#include <memoria/core/datatypes/buffer/ssrle_buffer.hpp>

namespace memoria {

template <size_t AlphabetSize_>
struct Sequence {
    static constexpr size_t AlphabetSize = AlphabetSize_;
};

template <size_t AlphabetSize, typename Profile>
struct ICtrApiTypes<Sequence<AlphabetSize>, Profile> {
    using CtrInputBuffer = IOSSRLEBuffer<AlphabetSize>;
};

template <size_t AlphabetSize>
struct TypeHash<Sequence<AlphabetSize>>: UInt64Value<HashHelper<3948258819287234, AlphabetSize>> {};

template <size_t AlphabetSize>
struct DataTypeTraits<Sequence<AlphabetSize>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<>;

    static constexpr bool HasTypeConstructors = false;
    static constexpr bool isSdnDeserializable = false;

    static void create_signature(SBuf& buf, const Sequence<AlphabetSize>& obj)
    {
        buf << "Sequence" << AlphabetSize;
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Sequence" << AlphabetSize;
    }
};

}
