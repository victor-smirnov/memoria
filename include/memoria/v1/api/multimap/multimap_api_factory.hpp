
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
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/api/common/ctr_api_btfl.hpp>

namespace memoria {
namespace v1 {

template <typename Key, typename Value>
class Multimap {
    Key key_;
    Value value_;
public:
    Multimap(): key_(), value_() {}

    Multimap(Key key, Value value):
        key_(key), value_(value)
    {}

    const Key& key() const {return key_;}
    const Value value() const {return value_;}
};


template <typename Key_, typename Value_, typename Profile>
struct ICtrApiTypes<Multimap<Key_, Value_>, Profile> {

    using Key = Key_;
    using Value = Value_;

    using IOVSchema = TL<
        TL<
            ICtrApiSubstream<Key, io::ColumnWise1D>
        >,
        TL<
            ICtrApiSubstream<Value, io::ColumnWise1D>
        >
    >;
};

template <typename Key, typename Value>
struct TypeHash<Multimap<Key, Value>>: UInt64Value<
    HashHelper<1102, TypeHashV<Key>, TypeHashV<Value>>
> {};

template <typename Key, typename Value>
struct DataTypeTraits<Multimap<Key, Value>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<Key, Value>;

    static constexpr size_t MemorySize        = sizeof(EmptyType);
    static constexpr bool IsParametrised      = true;
    static constexpr bool HasTypeConstructors = false;
    static constexpr bool isSdnDeserializable = false;

    static void create_signature(SBuf& buf, const Multimap<Key, Value>& obj)
    {
        buf << "Multimap<";

        DataTypeTraits<Key>::create_signature(buf, obj.key());
        buf << ", ";
        DataTypeTraits<Value>::create_signature(buf, obj.value());

        buf << ">";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Multimap<";

        DataTypeTraits<Key>::create_signature(buf);
        buf << ", ";
        DataTypeTraits<Value>::create_signature(buf);

        buf << ">";
    }
};

}}
