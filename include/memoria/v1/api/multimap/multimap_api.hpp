
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

#include <memoria/v1/api/common/ctr_api_btfl.hpp>
#include <memoria/v1/api/common/ctr_input_btss.hpp>

#include "multimap_input.hpp"

#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

template <typename Key_, typename Value_, typename Profile>
class CtrApi<Multimap<Key_, Value_>, Profile>: public CtrApiBTFLBase<Multimap<Key_, Value_>, Profile> {
public:
    using Key = Key_;
    using Value = Value_;
    using DataValue = std::tuple<Key, Value>;
private:
    using MapT = Multimap<Key, Value>;
    
    using Base = CtrApiBTFLBase<Multimap<Key, Value>, Profile>;
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:
    using typename Base::CtrSizeT;
    
    static constexpr int32_t DataStreams = 2;
    using CtrSizesT = CtrSizes<Profile, DataStreams + 1>;
    

    MMA1_DECLARE_CTRAPI_BASIC_METHODS()
    
    Iterator find(const Key& key);
    Iterator find_or_create(const Key& key);
    
    bool contains(const Key& key);
    bool remove(const Key& key);
    
    Iterator begin() {return Base::seq_begin();}
    Iterator end() {return Base::seq_begin();}
    Iterator seek(int64_t pos);
    int64_t size() const;
    
    Iterator assign(const Key& key, bt::BufferProducer<CtrIOBuffer>& values_producer);
    
    template <typename InputIterator, typename EndIterator>
    Iterator assign(const Key& key, const InputIterator& start, const EndIterator& end)
    {
        InputIteratorProvider<Value, InputIterator, EndIterator, CtrIOBuffer> provider(start, end);        
        return assign(key, provider);
    }
    
    
    CtrSizeT read(const Key& key, bt::BufferConsumer<CtrIOBuffer>& values_consumer, CtrSizeT start = 0, CtrSizeT length = std::numeric_limits<CtrSizeT>::max());
};


template <typename Key, typename Value, typename Profile> 
class IterApi<Multimap<Key, Value>, Profile>: public IterApiBTFLBase<Multimap<Key, Value>, Profile> {
    
    using Base = IterApiBTFLBase<Multimap<Key, Value>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
    
public:
    using typename Base::CtrSizeT;
    
    static constexpr int32_t DataStreams = CtrApi<Multimap<Key, Value>, Profile>::DataStreams;
    using CtrSizesT = typename CtrApi<Multimap<Key, Value>, Profile>::CtrSizesT;
    
    using DataValue = std::tuple<Key, Value>;
    
    MMA1_DECLARE_ITERAPI_BASIC_METHODS()
    
    Key key() const;
    Value value() const;
    
    bool is_end();
    
    bool next_key();
    void to_prev_key();
    int64_t skipFw(int64_t offset) const;
    
    int64_t count_values() const;
    int64_t run_pos() const;
    void insert_key(const Key& key);
    void insert_value(const Value& value);
    
    std::vector<Value> read_values(int64_t size = std::numeric_limits<int64_t>::max());
    
    int64_t remove(int64_t length = 1);

    int64_t values_size() const;

    bool is_found(const Key& key);

    bool to_values();

    int64_t read_keys(bt::BufferConsumer<CtrIOBuffer>& consumer, int64_t length = std::numeric_limits<int64_t>::max());
    
    CtrSizeT read_values(bt::BufferConsumer<CtrIOBuffer>& values_consumer, CtrSizeT start = 0, CtrSizeT length = std::numeric_limits<CtrSizeT>::max());
    
    CtrSizeT insert_values(bt::BufferProducer<CtrIOBuffer>& values_producer);
    CtrSizeT insert_entry(const Key& key, bt::BufferProducer<CtrIOBuffer>& values_producer);
};
    
}}
