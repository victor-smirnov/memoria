
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

#pragma once

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/encoding_traits.hpp>
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>



#include <memory>
#include <tuple>

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

template <typename Key, typename Value>
struct MapIterator {

    using KeyV   = typename DataTypeTraits<Key>::ValueType;
    using ValueV = typename DataTypeTraits<Value>::ValueType;

    virtual ~MapIterator() noexcept {}

    virtual KeyV key() const = 0;
    virtual ValueV value() const = 0;
    virtual bool is_end() const = 0;
    virtual void next() = 0;


};

template <typename Key, typename Value, typename Profile>
struct ICtrApi<Map<Key, Value>, Profile>: public CtrReferenceable {

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using DefaultIOSubstreams = TL<
        TL<
            ICtrApiSubstream<Key, io::ColumnWise>,
            ICtrApiSubstream<Value, io::RowWise>
        >
    >;

    using IOSubstreamsTL = typename CtrApiIOSubstreamsProvider<
        Map<Key, Value>, Profile, DefaultIOSubstreams
    >::Type;

    virtual int64_t map_size() const = 0;
    virtual void assign_key(KeyView key, ValueView value) = 0;
    virtual void remove_key(KeyView key) = 0;

    virtual CtrSharedPtr<MapIterator<Key, Value>> iterator() = 0;

    MMA1_DECLARE_ICTRAPI();
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
