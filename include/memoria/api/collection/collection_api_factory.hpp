
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

namespace memoria {

template <typename Key>
class Collection {
    Key key_;
public:
    Collection(): key_() {}
    Collection(Key key):
        key_(key)
    {}

    const Key& key() const {return key_;}
};

template <typename Key, typename Profile>
struct ICtrApiTypes<Collection<Key>, Profile> {};


template <typename Key>
struct TypeHash<Collection<Key>>: UInt64Value<
    HashHelper<3147713234196301, TypeHashV<Key>>
> {};

template <typename Key>
struct DataTypeTraits<Collection<Key>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<Key>;

    static constexpr bool HasTypeConstructors = false;
    static constexpr bool isSdnDeserializable = false;
};

}
