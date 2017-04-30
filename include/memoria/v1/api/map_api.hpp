
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

#include <memoria/v1/core/container/ctr_api_btss.hpp>

#include <memory>

namespace memoria {
namespace v1 {


    
template <typename Key, typename Value, typename Profile> 
class CtrApi<Map<Key, Value>, Profile>: public CtrApiBTSSBase<Map<Key, Value>, Profile> {
    using MapT = Map<Key, Value>;
    
    using Base = CtrApiBTSSBase<Map<Key, Value>, Profile>;
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:
    MMA1_DECLARE_CTRAPI_BTSS_BASIC_METHODS()
    
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
    using Base::read;
    using Base::insert;
    
    MMA1_DECLARE_ITERAPI_BTSS_BASIC_METHODS()
    
    Key key() const;
    Value value() const;
    
    void insert(const Key& key, const Value& value);
};
    
}}
