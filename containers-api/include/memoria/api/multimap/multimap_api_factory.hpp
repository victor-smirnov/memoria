
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

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/reflection/typehash.hpp>

#include <memoria/api/common/ctr_api_btfl.hpp>
#include <memoria/api/common/ctr_input_btfl.hpp>

namespace memoria {

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


template <typename Key, typename Value>
class MultimapInputBuffer: public FLCtrBatchInput<TL<
        TL<HermesDTBuffer<Key>>,
        TL<HermesDTBuffer<Value>>
>>{
    using Base = FLCtrBatchInput<TL<
        TL<HermesDTBuffer<Key>>,
        TL<HermesDTBuffer<Value>>
    >>;
public:
    MultimapInputBuffer(): Base() {}

    HermesDTBuffer<Key>& keys() {
        return Base::template get<0, 1>();
    }

    const HermesDTBuffer<Key>& keys() const {
        return Base::template get<0, 1>();
    }

    HermesDTBuffer<Value>& values() {
        return Base::template get<1, 1>();
    }

    const HermesDTBuffer<Value>& values() const {
        return Base::template get<1, 1>();
    }
};


template <typename Key, typename Value, typename Profile>
struct ICtrApiTypes<Multimap<Key, Value>, Profile> {
    using CtrInputBuffer = MultimapInputBuffer<Key, Value>;
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

    static constexpr bool HasTypeConstructors = false;
};

}
