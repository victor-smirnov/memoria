
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


    
template <typename Key, typename Profile> 
class CtrApi<Set<Key>, Profile>: public CtrApiBTSSBase<Set<Key>, Profile> {

    using Base = CtrApiBTSSBase<Set<Key>, Profile>;
    
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:

    MMA1_DECLARE_CTRAPI_BASIC_METHODS()
    
    
    Iterator begin();
    Iterator end();
    Iterator find(const Key& key);
    bool contains(const Key& key);
    bool remove(const Key& key);
    bool insert(const Key& key);
};


template <typename Key, typename Profile> 
class IterApi<Set<Key>, Profile>: public IterApiBTSSBase<Set<Key>, Profile> {
    
    using Base = IterApiBTSSBase<Set<Key>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
     
public:
    
    using Base::read;
    using Base::insert;
    
    MMA1_DECLARE_ITERAPI_BASIC_METHODS()
    
    Key key() const;
    void insert(const Key& key);
};
    
}}
