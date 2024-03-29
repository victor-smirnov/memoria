
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

#include <memoria/core/arena/map.hpp>
#include <memoria/core/arena/tiny_map.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/datatype.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/hermes/map/generic_map.hpp>

namespace memoria {
namespace hermes {

struct FSEKeySubtype {};

template <typename KeyDT, typename ValueDT, typename SubtypeSelector, bool StatelessType>
class TypedMapData;

template <typename EntryT, typename MapT, typename Iterator>
class MapIteratorAccessor {
    MapT map_;
    Iterator iterator_;

public:
    using ViewType = EntryT;

    MapIteratorAccessor(const MapT& map, Iterator iterator):
        map_(map), iterator_(iterator)
    {}

    MapT& map() {return map_;}
    const MapT& map() const {return map_;}

    EntryT current() const {
        return EntryT(&iterator_, map_.mem_holder());
    }

    bool operator==(const MapIteratorAccessor&other) const noexcept {
        return iterator_ == other.iterator_;
    }

    void next() {
        iterator_.next();
    }
};

template <typename Fn>
void GenericMap::for_each(Fn&& fn) const
{
    auto ii = iterator();
    while (!ii->is_end())
    {
        fn(ii->key(), ii->value());
        ii->next();
    }
}

}}
