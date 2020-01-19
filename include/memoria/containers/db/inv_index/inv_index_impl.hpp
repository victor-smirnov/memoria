
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

#include <memoria/containers/db/inv_index/inv_index_factory.hpp>

#include <memoria/api/db/inv_index/inv_index_api.hpp>
#include <memoria/api/db/inv_index/inv_index_input.hpp>

#include <memoria/core/container/ctr_impl_btfl.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memory>

namespace memoria {

/*
template <typename Profile>
int64_t CtrApi<InvertexIndex, Profile>::size() const
{
    return this->pimpl_->size();
}


template <typename Key, typename Value, typename Profile>
typename CtrApi<Multimap<Key, Value>, Profile>::Iterator CtrApi<Multimap<Key, Value>, Profile>::find(const Key& key) 
{
    return this->pimpl_->find(key);
}

template <typename Key, typename Value, typename Profile>
typename CtrApi<Multimap<Key, Value>, Profile>::Iterator CtrApi<Multimap<Key, Value>, Profile>::find_or_create(const Key& key) 
{
    return this->pimpl_->find_or_create(key);
}


template <typename Key, typename Value, typename Profile>
bool CtrApi<Multimap<Key, Value>, Profile>::contains(const Key& key) 
{
    auto iter = this->pimpl_->find(key);
    
    return iter->is_found(key);
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Multimap<Key, Value>, Profile>::remove(const Key& key) 
{
    auto ii = this->pimpl_->find(key);
    
    if (ii->is_found(key)) {
        ii->remove(1);
        return true;
    }
    
    return false;
}



template <typename Key, typename Value, typename Profile>
typename CtrApi<Multimap<Key, Value>, Profile>::CtrSizeT 
CtrApi<Multimap<Key, Value>, Profile>::read(
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


template <typename Key, typename Value, typename Profile>
typename CtrApi<Multimap<Key, Value>, Profile>::Iterator 
CtrApi<Multimap<Key, Value>, Profile>::assign(
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







template <typename Key, typename Value, typename Profile>
Key IterApi<Multimap<Key, Value>, Profile>::key() const
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


template <typename Key, typename Value, typename Profile>
Value IterApi<Multimap<Key, Value>, Profile>::value() const
{
    return _::ValueHelper<decltype(this->pimpl_->value())>::convert(this->pimpl_->value());
}


template <typename Key, typename Value, typename Profile>
bool IterApi<Multimap<Key, Value>, Profile>::is_end()
{
    return this->pimpl_->is_end();
}


template <typename Key, typename Value, typename Profile>
bool IterApi<Multimap<Key, Value>, Profile>::next_key()
{
    return this->pimpl_->next_key();
}

template <typename Key, typename Value, typename Profile>
void IterApi<Multimap<Key, Value>, Profile>::to_prev_key()
{
    return this->pimpl_->to_prev_key();
}

template <typename Key, typename Value, typename Profile>
int64_t IterApi<Multimap<Key, Value>, Profile>::iter_skip_fw(int64_t offset) const
{
    return this->pimpl_->iter_skip_fw(offset);
}

template <typename Key, typename Value, typename Profile>
int64_t IterApi<Multimap<Key, Value>, Profile>::count_values() const
{
    return this->pimpl_->count_values();
}

template <typename Key, typename Value, typename Profile>
int64_t IterApi<Multimap<Key, Value>, Profile>::run_pos() const
{
    return this->pimpl_->run_pos();
}
 
 
template <typename Key, typename Value, typename Profile>
void IterApi<Multimap<Key, Value>, Profile>::insert_key(const Key& key)
{
    return this->pimpl_->insert_key(key);
} 


template <typename Key, typename Value, typename Profile>
void IterApi<Multimap<Key, Value>, Profile>::insert_value(const Value& value)
{
    return this->pimpl_->insert_value(value);
} 


template <typename Key, typename Value, typename Profile>
std::vector<Value> IterApi<Multimap<Key, Value>, Profile>::read_values(int64_t size) 
{
    return this->pimpl_->read_values(size);
}
 
 
template <typename Key, typename Value, typename Profile>
int64_t IterApi<Multimap<Key, Value>, Profile>::remove(int64_t length)
{
    return this->pimpl_->remove(length).sum();
}  



template <typename Key, typename Value, typename Profile>
bool IterApi<Multimap<Key, Value>, Profile>::is_found(const Key& key)
{
    return this->pimpl_->is_found(key);
}

template <typename Key, typename Value, typename Profile>
bool IterApi<Multimap<Key, Value>, Profile>::to_values()
{
    return this->pimpl_->to_values();
}

template <typename Key, typename Value, typename Profile>
int64_t IterApi<Multimap<Key, Value>, Profile>::read_keys(bt::BufferConsumer<CtrIOBuffer>& consumer, int64_t length)
{
    return this->pimpl_->read_keys(&consumer, length);
}

template <typename Key, typename Value, typename Profile>
typename IterApi<Multimap<Key, Value>, Profile>::CtrSizeT 
IterApi<Multimap<Key, Value>, Profile>::read_values(
    bt::BufferConsumer<CtrIOBuffer>& values_consumer, CtrSizeT start, CtrSizeT length)
{
    return this->pimpl_->read_values(values_consumer, start, length);
}


template <typename Key, typename Value, typename Profile>
typename IterApi<Multimap<Key, Value>, Profile>::CtrSizeT 
IterApi<Multimap<Key, Value>, Profile>::insert_values(
    bt::BufferProducer<CtrIOBuffer>& values_producer)
{
    return this->pimpl_->insert_values(values_producer);
}

template <typename Key, typename Value, typename Profile>
typename IterApi<Multimap<Key, Value>, Profile>::CtrSizeT 
IterApi<Multimap<Key, Value>, Profile>::insert_entry(
    const Key& key,
    bt::BufferProducer<CtrIOBuffer>& values_producer
)
{
    return this->pimpl_->insert_mmap_entry(key, values_producer);
}
*/


}
