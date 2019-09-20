
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

#include <memoria/v1/api/map/map_api.hpp>

#include <memory>

namespace memoria {
namespace v1 {


namespace _ {
    template <typename T>
    struct MapValueHelper {
        template <typename TT>
        static T convert(TT&& value) {
            return value;
        }
    };

    template <typename T>
    struct MapValueHelper<StaticVector<T, 1>> {
        template <typename TT>
        static T convert(TT&& value) {
            return value[0];
        }
    };
}

template <typename Key, typename Value, typename IteratorPtr>
class MapIteratorImpl: public MapIterator<Key, Value> {

    using KeyV   = typename DataTypeTraits<Key>::ValueType;
    using ValueV = typename DataTypeTraits<Value>::ValueType;

    IteratorPtr iter_;
public:
    MapIteratorImpl(IteratorPtr iter):
        iter_(iter)
    {}

    virtual KeyV key() const
    {
        return iter_->key();
    }

    virtual ValueV value() const
    {
        using ValueTT = decltype(iter_->value());
        return _::MapValueHelper<ValueTT>::convert(iter_->value());
    }

    virtual bool is_end() const
    {
        return iter_->iter_is_end();
    }

    virtual void next() {
        iter_->next();
    }

    void dump() const {
        iter_->dump();
    }
};

}}
