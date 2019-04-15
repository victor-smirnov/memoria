
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
#include <memoria/v1/api/db/edge_map/edge_map_input.hpp>

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


#ifdef MMA1_USE_IOBUFFER

template <typename Profile>
typename CtrApi<EdgeMap, Profile>::CtrSizeT
CtrApi<EdgeMap, Profile>::read(
    const Key& key,
    bt::BufferConsumer<CtrIOBuffer>& values_consumer, 
    CtrSizeT start, 
    CtrSizeT length
)
{
    auto ii = find(key);
    if (ii.is_found(key)) 
    {
        return ii.read_values(values_consumer);
    }
    
    return 0;
}


template <typename Profile>
typename CtrApi<EdgeMap, Profile>::Iterator
CtrApi<EdgeMap, Profile>::assign(
    const Key& key,
    bt::BufferProducer<CtrIOBuffer>& values_producer
)
{
    auto ii = find(key);
    
    if (ii.is_found(key)) 
    {
        auto size = ii.count_values();
        ii.next_sym();
        if (size > 0) 
        {
            ii.remove(size);
        }
        
        ii.insert_values(values_producer);
        
        return ii;
    }
    
    ii.insert_entry(key, values_producer);
    
    return ii;
}

#endif

template <typename Profile>
typename CtrApi<EdgeMap, Profile>::EdgeMapValueIterator
CtrApi<EdgeMap, Profile>::EdgeMapKeyIterator::values()
{
    auto ii = iterator_.clone();
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
int64_t IterApi<EdgeMap, Profile>::skipFw(int64_t offset) const
{
    return this->pimpl_->skipFw(offset);
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
std::vector<typename IterApi<EdgeMap, Profile>::Value> IterApi<EdgeMap, Profile>::read_values(int64_t size)
{
    return this->pimpl_->read_values(size);
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

#ifdef MMA1_USE_IOBUFFER

template <typename Profile>
int64_t IterApi<EdgeMap, Profile>::read_keys(bt::BufferConsumer<CtrIOBuffer>& consumer, int64_t length)
{
    return this->pimpl_->read_keys(&consumer, length);
}

template <typename Profile>
typename IterApi<EdgeMap, Profile>::CtrSizeT
IterApi<EdgeMap, Profile>::read_values(
    bt::BufferConsumer<CtrIOBuffer>& values_consumer, CtrSizeT start, CtrSizeT length)
{
    return this->pimpl_->read_values(values_consumer, start, length);
}


template <typename Profile>
typename IterApi<EdgeMap, Profile>::CtrSizeT
IterApi<EdgeMap, Profile>::insert_values(
    bt::BufferProducer<CtrIOBuffer>& values_producer)
{
    return this->pimpl_->insert_values(values_producer);
}

template <typename Profile>
typename IterApi<EdgeMap, Profile>::CtrSizeT
IterApi<EdgeMap, Profile>::insert_entry(
    const Key& key,
    bt::BufferProducer<CtrIOBuffer>& values_producer
)
{
    return this->pimpl_->insert_mmap_entry(key, values_producer);
}

#endif

template <typename Profile>
EdgeMapFindResult IterApi<EdgeMap, Profile>::find_value(const Value& value) {
    return this->pimpl_->find_value(value);
}

}}
