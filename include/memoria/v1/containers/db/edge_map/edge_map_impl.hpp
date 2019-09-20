
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

#include <memoria/v1/containers/db/edge_map/edge_map_factory.hpp>

#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>

#include <memoria/v1/core/container/ctr_impl_btfl.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Profile>
int64_t CtrApi<EdgeMap, Profile>::size() const
{
    return this->pimpl_->size();
}


template <typename Profile>
typename CtrApi<EdgeMap, Profile>::Iterator CtrApi<EdgeMap, Profile>::find(const Key& key)
{
    return this->pimpl_->find(key);
}

template <typename Profile>
typename CtrApi<EdgeMap, Profile>::Iterator CtrApi<EdgeMap, Profile>::find_or_create(const Key& key)
{
    return this->pimpl_->find_or_create(key);
}


template <typename Profile>
bool CtrApi<EdgeMap, Profile>::contains(const Key& key)
{
    auto iter = this->pimpl_->find(key);
    
    return iter->is_found(key);
}

template <typename Profile>
bool CtrApi<EdgeMap, Profile>::contains(const Key& key, const Value& value)
{
    auto iter = this->pimpl_->find(key);

    return iter->contains(value);
}

template <typename Profile>
bool CtrApi<EdgeMap, Profile>::remove(const Key& key)
{
    auto ii = this->pimpl_->find(key);
    
    if (ii->is_found(key)) {
        ii->remove(1);
        return true;
    }
    
    return false;
}

template <typename Profile>
bool CtrApi<EdgeMap, Profile>::remove(const Key& key, const Value& value)
{
    return this->pimpl_->remove(key, value);
}

template <typename Profile>
bool CtrApi<EdgeMap, Profile>::upsert(const Key& key, const Value& value)
{
    return this->pimpl_->upsert(key, value);
}

template <typename Profile>
typename CtrApi<EdgeMap, Profile>::EdgeMapValueIterator CtrApi<EdgeMap, Profile>::iterate(const Key& key)
{
    auto ii = this->find(key);
    if (ii.is_found(key))
    {
        auto size = ii.count_values();
        if (size > 0)
        {
            ii.to_values();
            return EdgeMapValueIterator(ii, size);
        }
        else {
            return EdgeMapValueIterator();
        }
    }
    else
    {
        return EdgeMapValueIterator();
    }
}


template <typename Profile>
typename CtrApi<EdgeMap, Profile>::EdgeMapValueIterator
CtrApi<EdgeMap, Profile>::EdgeMapKeyIterator::values()
{
    auto ii = iterator_.iter_clone();
    auto size = ii.count_values();
    if (size > 0)
    {
        ii.to_values();
        return EdgeMapValueIterator(ii, size);
    }
    else {
        return EdgeMapValueIterator();
    }
}


template <typename Profile>
typename IterApi<EdgeMap, Profile>::Key IterApi<EdgeMap, Profile>::key() const
{
    return this->pimpl_->key();
}




namespace _ {
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


template <typename Profile>
typename IterApi<EdgeMap, Profile>::Value IterApi<EdgeMap, Profile>::value() const
{
    return _::ValueHelper<decltype(this->pimpl_->value())>::convert(this->pimpl_->value());
}


template <typename Profile>
bool IterApi<EdgeMap, Profile>::is_end()
{
    return this->pimpl_->is_end();
}


template <typename Profile>
bool IterApi<EdgeMap, Profile>::next_key()
{
    return this->pimpl_->next_key();
}

template <typename Profile>
void IterApi<EdgeMap, Profile>::to_prev_key()
{
    return this->pimpl_->to_prev_key();
}

template <typename Profile>
int64_t IterApi<EdgeMap, Profile>::iter_skip_fw(int64_t offset) const
{
    return this->pimpl_->iter_skip_fw(offset);
}

template <typename Profile>
int64_t IterApi<EdgeMap, Profile>::count_values() const
{
    return this->pimpl_->count_values();
}

template <typename Profile>
int64_t IterApi<EdgeMap, Profile>::run_pos() const
{
    return this->pimpl_->run_pos();
}
 
 
template <typename Profile>
void IterApi<EdgeMap, Profile>::insert_key(const Key& key)
{
    return this->pimpl_->insert_key(key);
} 


template <typename Profile>
void IterApi<EdgeMap, Profile>::insert_value(const Value& value)
{
    return this->pimpl_->insert_value(value);
} 


 
template <typename Profile>
int64_t IterApi<EdgeMap, Profile>::remove(int64_t length)
{
    return this->pimpl_->remove(length).sum();
}  



template <typename Profile>
bool IterApi<EdgeMap, Profile>::is_found(const Key& key)
{
    return this->pimpl_->is_found(key);
}

template <typename Profile>
bool IterApi<EdgeMap, Profile>::to_values()
{
    return this->pimpl_->to_values();
}


template <typename Profile>
EdgeMapFindResult IterApi<EdgeMap, Profile>::find_value(const Value& value) {
    return this->pimpl_->find_value(value);
}

}}
