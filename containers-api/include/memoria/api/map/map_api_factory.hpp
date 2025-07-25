
// Copyright 2019-2025 Victor Smirnov
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
class MapInputBuffer: public CtrBatchInputBase<TL<TL<
        HermesDTBuffer<Key>,
        HermesDTBuffer<Value>
>>>{
    using Base = CtrBatchInputBase<TL<TL<
        HermesDTBuffer<Key>,
        HermesDTBuffer<Value>
    >>>;
public:
    MapInputBuffer(): Base() {}

    HermesDTBuffer<Key>& keys() {
        return Base::template get<0, 1>();
    }

    const HermesDTBuffer<Key>& keys() const {
        return Base::template get<0, 1>();
    }

    HermesDTBuffer<Value>& values() {
        return Base::template get<0, 2>();
    }

    const HermesDTBuffer<Value>& values() const {
        return Base::template get<0, 2>();
    }
};


template <typename Key, typename Value, typename Profile>
struct ICtrApiTypes<Map<Key, Value>, Profile> {
    using CtrInputBuffer = MapInputBuffer<Key, Value>;
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

    static constexpr bool HasTypeConstructors = false;
};

}
