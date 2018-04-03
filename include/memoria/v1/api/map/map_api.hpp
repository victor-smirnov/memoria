
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {


    
template <typename Key_, typename Value_, typename Profile> 
class CtrApi<Map<Key_, Value_>, Profile>: public CtrApiBTSSBase<Map<Key_, Value_>, Profile> {
public:
    using Key = Key_;
    using Value = Value_;
    using DataValue = std::tuple<Key, Value>;
private:
    using MapT = Map<Key, Value>;
    
    using Base = CtrApiBTSSBase<Map<Key, Value>, Profile>;
public:    
    
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
    MMA1_DECLARE_CTRAPI_BASIC_METHODS()
    
    Iterator find(const Key& key);
    bool contains(const Key& key);
    bool remove(const Key& key);
    
    Iterator assign(const Key& key, const Value& value);
};


template <typename Key, typename Value, typename Profile> 
class IterApi<Map<Key, Value>, Profile>: public IterApiBTSSBase<Map<Key, Value>, Profile> {
    
    using Base = IterApiBTSSBase<Map<Key, Value>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
    
public:
    using DataValue = std::tuple<Key, Value>;
    
    using Base::read;
    using Base::insert;
    
    MMA1_DECLARE_ITERAPI_BASIC_METHODS()
    
    Key key() const;
    Value value() const;
    
    void insert(const Key& key, const Value& value);
    
    
    template <typename KeyT, typename ValueT>
    auto insert(const std::vector<std::tuple<KeyT, ValueT>>& data) 
    {
        using StdIter = typename std::vector<std::tuple<KeyT, ValueT>>::const_iterator;
        InputIteratorProvider<DataValue, StdIter, StdIter, CtrIOBuffer> provider(data.begin(), data.end());
        return insert(provider);
    }
    
    template <typename Iterator, typename EndIterator>
    auto insert(const Iterator& iter, const EndIterator& end) 
    {
        InputIteratorProvider<DataValue, Iterator, EndIterator, CtrIOBuffer> provider(iter, end);
        return insert(provider);
    }
    
    template <typename Fn>
    auto insert_fn(int64_t size, Fn&& fn) 
    {
        using Iterator = IteratorFn<std::remove_reference_t<decltype(fn)>>;
        
        Iterator fni(fn);
        EndIteratorFn<int64_t> endi(size);
        
        InputIteratorProvider<DataValue, Iterator, EndIteratorFn<int64_t>, CtrIOBuffer> provider(fni, endi);
        return insert(provider);
    }
    
    std::vector<DataValue> read(size_t size);
};
    
}}
