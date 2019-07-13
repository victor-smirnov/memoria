
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

#include "mmap_factory.hpp"

#include <memoria/v1/api/multimap/multimap_api.hpp>
#include <memoria/v1/api/multimap/multimap_input.hpp>
#include <memoria/v1/core/container/ctr_impl_btfl.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Key, typename Value, typename Profile>
int64_t CtrApi<Multimap<Key, Value>, Profile>::size() const
{
    return this->pimpl_->size();
}



template <typename Key, typename Value, typename Profile>
CtrSharedPtr<IValuesIterator<Value>> CtrApi<Multimap<Key, Value>, Profile>::find(Key key)
{
    return this->pimpl_->find_v(key);
}

template <typename Key, typename Value, typename Profile>
CtrSharedPtr<IEntriesIterator<Key,Value>> CtrApi<Multimap<Key, Value>, Profile>::seek(CtrSizeT pos)
{
    return this->pimpl_->seek_e(pos);
}

template <typename Key, typename Value, typename Profile>
CtrSharedPtr<IKeysIterator<Key,Value>> CtrApi<Multimap<Key, Value>, Profile>::keys()
{
    return this->pimpl_->keys();
}


template <typename Key, typename Value, typename Profile>
void CtrApi<Multimap<Key, Value>, Profile>::append_entries(io::IOVectorProducer& producer)
{
    this->pimpl_->end()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
}

template <typename Key, typename Value, typename Profile>
void CtrApi<Multimap<Key, Value>, Profile>::append_entry(Key key, absl::Span<const Value> values)
{
    io::MultimapEntryIOVector<Key, Value> iovector(&key, values.data(), values.size());
    this->pimpl_->end()->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());
}



template <typename Key, typename Value, typename Profile>
void CtrApi<Multimap<Key, Value>, Profile>::prepend_entries(io::IOVectorProducer& producer)
{
    this->pimpl_->begin()->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
}

template <typename Key, typename Value, typename Profile>
void CtrApi<Multimap<Key, Value>, Profile>::prepend_entry(Key key, absl::Span<const Value> values)
{
    io::MultimapEntryIOVector<Key, Value> iovector(&key, values.data(), values.size());
    this->pimpl_->begin()->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());
}



template <typename Key, typename Value, typename Profile>
void CtrApi<Multimap<Key, Value>, Profile>::insert_entries(Key before, io::IOVectorProducer& producer)
{
    auto ii = this->pimpl_->find(before);
    ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());
}

template <typename Key, typename Value, typename Profile>
void CtrApi<Multimap<Key, Value>, Profile>::insert_entry(Key before, Key key, absl::Span<const Value> values)
{
    io::MultimapEntryIOVector<Key, Value> iovector(&key, values.data(), values.size());

    auto ii = this->pimpl_->find(before);
    ii->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());
}


template <typename Key, typename Value, typename Profile>
bool CtrApi<Multimap<Key, Value>, Profile>::upsert(Key key, io::IOVectorProducer& producer)
{
    auto ii = this->pimpl_->find(key);

    if (ii->is_found(key))
    {
        ii->remove(1);
        ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());

        return true;
    }

    ii->insert_iovector(producer, 0, std::numeric_limits<int64_t>::max());

    return false;
}


template <typename Key, typename Value, typename Profile>
bool CtrApi<Multimap<Key, Value>, Profile>::upsert(Key key, absl::Span<const Value> values)
{
    auto ii = this->pimpl_->find(key);

    io::MultimapEntryIOVector<Key, Value> iovector(&key, values.data(), values.size());

    if (ii->is_found(key))
    {
        ii->remove(1);
        ii->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());

        return true;
    }

    ii->insert_iovector(iovector, 0, std::numeric_limits<int64_t>::max());

    return false;
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
bool CtrApi<Multimap<Key, Value>, Profile>::remove_all(const Key& from, const Key& to)
{
    auto ii = this->pimpl_->find(from);
    auto jj = this->pimpl_->find(to);

    return ii->remove_all(*jj.get());
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Multimap<Key, Value>, Profile>::remove_from(const Key& from)
{
    auto ii = this->pimpl_->find(from);
    auto jj = this->pimpl_->end();

    return ii->remove_all(*jj.get());
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Multimap<Key, Value>, Profile>::remove_before(const Key& to)
{
    auto ii = this->pimpl_->begin();
    auto jj = this->pimpl_->find(to);

    return ii->remove_all(*jj.get());
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
int64_t IterApi<Multimap<Key, Value>, Profile>::skipFw(int64_t offset) const
{
    return this->pimpl_->skipFw(offset);
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
int64_t IterApi<Multimap<Key, Value>, Profile>::key_pos() const
{
    return this->pimpl_->key_pos();
}
 
 
template <typename Key, typename Value, typename Profile>
void IterApi<Multimap<Key, Value>, Profile>::insert_key(const Key& key)
{
    return this->pimpl_->insert_key(key);
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



}}
