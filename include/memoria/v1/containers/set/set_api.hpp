
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

#include <memoria/v1/core/container/ctr_api.hpp>


#include <memory>

namespace memoria {
namespace v1 {


    
template <typename Key, typename Profile> 
class CtrApi<Set<Key>, Profile> {
    using AllocatorT = IWalkableAllocator<ProfilePageType<Profile>>;
    using CtrT       = SharedCtr2<Set<Key>, AllocatorT, Profile>;
    using CtrPtr = std::shared_ptr<CtrT>;

    using Iterator = IterApi<Set<Key>, Profile>;
    
    
    
    CtrPtr pimpl_;
    
public:
    CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name);
    ~CtrApi();
    
    CtrApi(const CtrApi&);
    CtrApi(CtrApi&&);
    
    CtrApi& operator=(const CtrApi&);
    CtrApi& operator=(CtrApi&&);
    
    bool operator==(const CtrApi& other) const;
    
    operator bool() const;
    
    BigInt size();
    Iterator begin();
    Iterator end();
    Iterator find(const Key& key);
    bool contains(const Key& key);
    bool remove(const Key& key);
    bool insert(const Key& key);
    
    UUID name();
    
    static void init();
    static const ContainerMetadataPtr& metadata();
};


template <typename Key, typename Profile> 
class IterApi<Set<Key>, Profile> {
    
    using IterT = SharedIter<Set<Key>, Profile>;
    using IterPtr = std::shared_ptr<IterT>;
     
    IterPtr pimpl_;
    
public:
    IterApi(IterPtr ptr);
    
    IterApi(const IterApi&);
    IterApi(IterApi&&);
    ~IterApi();
    
    IterApi& operator=(const IterApi&);
    IterApi& operator=(IterApi&&);
    
    bool operator==(const IterApi& other) const;
    
    operator bool() const;
    
    bool is_end() const;
    Key key() const;
    
    bool next();
    bool prev();
    
    void remove();
    void insert(const Key& key);
    
    BigInt for_each(std::function<bool(const Key&)>);
};
    
}}
