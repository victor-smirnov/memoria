
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

template <typename Key, typename Value>
class Map {
    Key key_;
    Value value_;
public:
    Map(): key_(), value_() {}

    Map(Key key, Value value):
        key_(key), value_(value)
    {}

    const Key& key() const {return key_;}
    const Value value() const {return value_;}
};

template <typename Key_, typename Value_, typename Profile>
struct ICtrApiTypes<Map<Key_, Value_>, Profile> {

    using Key = Key_;
    using Value = Value_;

    using IOVSchema = TL<
        TL<
            ICtrApiSubstream<Key, io::ColumnWise>,
            ICtrApiSubstream<Value, io::RowWise>
        >
    >;
};


template <typename Key, typename Value>
struct TypeHash<Map<Key, Value>>: UInt64Value<
    HashHelper<1100, TypeHashV<Key>, TypeHashV<Value>>
> {};

template <typename Key, typename Value>
struct DataTypeTraits<Map<Key, Value>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<Key, Value>;

    static constexpr size_t MemorySize        = sizeof(EmptyType);
    static constexpr bool IsParametrised      = true;
    static constexpr bool HasTypeConstructors = false;
    static constexpr bool isSdnDeserializable = false;

    static void create_signature(SBuf& buf, const Map<Key, Value>& obj)
    {
        buf << "Map<";

        DataTypeTraits<Key>::create_signature(buf, obj.key());
        buf << ", ";
        DataTypeTraits<Value>::create_signature(buf, obj.value());

        buf << ">";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Map<";

        DataTypeTraits<Key>::create_signature(buf);
        buf << ", ";
        DataTypeTraits<Value>::create_signature(buf);

        buf << ">";
    }
};

}}
