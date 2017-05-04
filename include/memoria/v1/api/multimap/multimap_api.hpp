
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

#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

template <typename Key, typename Value> class Multimap {};   
    
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

    MMA1_DECLARE_CTRAPI_BASIC_METHODS()
    
    Iterator find(const Key& key);
    bool contains(const Key& key);
    bool remove(const Key& key);
    
    //Iterator assign(const Key& key, const Value& value);
};


template <typename Key, typename Value, typename Profile> 
class IterApi<Multimap<Key, Value>, Profile>: public IterApiBTFLBase<Multimap<Key, Value>, Profile> {
    
    using Base = IterApiBTSSBase<Multimap<Key, Value>, Profile>;
    
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
    std::vector<DataValue> read(size_t size);
};
    
}}
