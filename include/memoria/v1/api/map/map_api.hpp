
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

    virtual ~MapIterator() noexcept {}

    virtual Key key() const = 0;
    virtual Value value() const = 0;
    virtual bool is_end() const = 0;
    virtual void next() = 0;


};

template <typename Key_, typename Value_, typename Profile>
struct ICtrApi<Map<Key_, Value_>, Profile>: public CtrReferenceable {

    using Key = Key_;
    using Value = Value_;

    using DefaultIOSubstreams = TL<
        TL<
            //io::IOSubstreamTF<io::IOColumnwiseFixedSizeArraySubstream>
        >
    >;

    using IOSubstreamsTL = typename CtrApiIOSubstreamsProvider<Map<Key_, Value_>, Profile, DefaultIOSubstreams>::Type;

    virtual int64_t map_size() const = 0;
    virtual void assign_key(Key key, Value value) = 0;
    virtual void remove_key(Key key) = 0;

    virtual CtrSharedPtr<MapIterator<Key_, Value_>> iterator() = 0;
};


    
template <typename Key_, typename Value_, typename Profile> 
class CtrApi<Map<Key_, Value_>, Profile>: public CtrApiBTSSBase<Map<Key_, Value_>, Profile> {
public:
    using Key = Key_;
    using Value = Value_;
    using DataValue = std::tuple<Key, Value>;
private:
    using MapT = Map<Key, Value>;
    
    using Base = CtrApiBTSSBase<Map<Key, Value>, Profile>;
public:    
    using typename Base::CtrID;
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
    MMA1_DECLARE_CTRAPI_BASIC_METHODS()
    
    Iterator find(const Key& key);
    bool contains(const Key& key);
    bool remove(const Key& key);
    
    Iterator assign(const Key& key, const Value& value);
};


template <typename Key, typename Value, typename Profile> 
class IterApi<Map<Key, Value>, Profile>: public IterApiBTSSBase<Map<Key, Value>, Profile> {
    
    using Base = IterApiBTSSBase<Map<Key, Value>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
    
public:
    using Base::insert;

    using DataValue = std::tuple<Key, Value>;
    
    MMA1_DECLARE_ITERAPI_BASIC_METHODS()
    
    Key key() const;
    Value value() const;
    
    void insert(const Key& key, const Value& value);
    void assign(const Value& value);
    
    bool is_found(const Key& key) const;
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
