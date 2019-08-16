
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

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

namespace memoria {
namespace v1 {

template <typename Key>
class Set {
    Key key_;
public:
    Set(Key key):
        key_(key)
    {}

    const Key& key() const {return key_;}
};

template <typename Key_, typename Profile>
struct ICtrApiTypes<Set<Key_>, Profile> {

    using Key = Key_;

    using IOVSchema = TL<
        TL<
            ICtrApiSubstream<Key, io::ColumnWise>
        >
    >;
};


template <typename Key>
struct TypeHash<Set<Key>>: UInt64Value<
    HashHelper<1101, TypeHashV<Key>>
> {};

template <typename Key>
struct DataTypeTraits<Set<Key>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<Key>;

    static constexpr size_t MemorySize        = sizeof(EmptyType);
    static constexpr bool IsParametrised      = true;
    static constexpr bool HasTypeConstructors = false;

    static void create_signature(SBuf& buf, const Set<Key>& obj)
    {
        buf << "Set<";
        DataTypeTraits<Key>::create_signature(buf, obj.key());
        buf << ">";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Set<";
        DataTypeTraits<Key>::create_signature(buf);
        buf << ">";
    }
};

}}
