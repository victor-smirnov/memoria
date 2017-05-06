
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

#include "map_factory.hpp"

#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/core/container/ctr_impl_btss.hpp>
#include <memoria/v1/core/tools/static_array.hpp>



#include <memory>

namespace memoria {
namespace v1 {




template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::find(const Key& key) 
{
    return this->pimpl_->find(key);
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::contains(const Key& key) 
{
    auto iter = this->pimpl_->find(key);
    
    if (!iter->isEnd()) {
        return iter->key() == key;
    }
    
    return false;
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::remove(const Key& key) 
{
    return this->pimpl_->remove(key);
}

template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::assign(const Key& key, const Value& value) 
{
    return this->pimpl_->assign(key, value);
}







template <typename Key, typename Value, typename Profile>
Key IterApi<Map<Key, Value>, Profile>::key() const
{
    return this->pimpl_->key();
}


namespace detail01 {
    template <typename T>
    struct ValueHelper {
        template <typename TT>
        static T convert(TT&& value) {
            return value;
        }
    };
    
    template <typename T>
    struct ValueHelper<StaticVector<T, 1>> {
        template <typename TT>
        static T convert(TT&& value) {
            return value[0];
        }
    };
} 


template <typename Key, typename Value, typename Profile>
Value IterApi<Map<Key, Value>, Profile>::value() const
{
    return detail01::ValueHelper<decltype(this->pimpl_->value())>::convert(this->pimpl_->value());
}



template <typename Key, typename Value, typename Profile>
void IterApi<Map<Key, Value>, Profile>::insert(const Key& key, const Value& value)
{
    return this->pimpl_->insert(key, value);
}

template <typename Key, typename Value, typename Profile>
std::vector<typename IterApi<Map<Key, Value>, Profile>::DataValue> IterApi<Map<Key, Value>, Profile>::read(size_t size) 
{
    std::vector<DataValue> data;
    
    auto fn = [&](const DataValue& vv){
        data.emplace_back(vv);
    };
    
    BTSSAdaptorFn<DataValue, std::remove_reference_t<decltype(fn)>, CtrIOBuffer> consumer(fn);
    
    this->read(consumer);
    
    return data;
}

    
}}
