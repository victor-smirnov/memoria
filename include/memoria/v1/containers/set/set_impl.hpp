
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

#include "set_api.hpp"
#include "set_factory.hpp"

#include <memoria/v1/core/container/ctr_impl.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Key, typename Profile>
using SharedSet = SharedCtr2<Set<Key>, IWalkableAllocator<ProfilePageType<Profile>>, Profile>;
    
    
template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name):
    pimpl_(std::make_shared<SharedSet<Key, Profile>>(allocator, command, name))
{}

template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::CtrApi(const CtrApi& other):pimpl_(other.pimpl_) {}


template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::CtrApi(CtrApi&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::~CtrApi() {}



template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>& CtrApi<Set<Key>, Profile>::operator=(const CtrApi& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}


template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>& CtrApi<Set<Key>, Profile>::operator=(CtrApi&& other) 
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::operator==(const CtrApi& other) const 
{
    return pimpl_ == other.pimpl_;
}

template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}


template <typename Key, typename Profile>
BigInt CtrApi<Set<Key>, Profile>::size()
{
    return pimpl_->size();
}


template <typename Key, typename Profile>
typename CtrApi<Set<Key>, Profile>::Iterator CtrApi<Set<Key>, Profile>::begin() 
{
    return pimpl_->begin();
}


template <typename Key, typename Profile>
typename CtrApi<Set<Key>, Profile>::Iterator CtrApi<Set<Key>, Profile>::end() 
{
    return pimpl_->end();
}

template <typename Key, typename Profile>
typename CtrApi<Set<Key>, Profile>::Iterator CtrApi<Set<Key>, Profile>::find(const Key& key) 
{
    return pimpl_->find(key);
}

template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::contains(const Key& key) 
{
    return pimpl_->contains(key);
}

template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::remove(const Key& key) 
{
    return pimpl_->remove(key);
}

template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::insert(const Key& key) 
{
    return pimpl_->insert_key(key);
}

template <typename Key, typename Profile>
UUID CtrApi<Set<Key>, Profile>::name() 
{
    return pimpl_->name();
}



template <typename Key, typename Profile>
void CtrApi<Set<Key>, Profile>::initMetadata()
{
    CtrT::initMetadata();
}






template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::IterApi(IterPtr ptr): pimpl_(ptr) {}


template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::IterApi(const IterApi& other): pimpl_(other.pimpl_) {}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::IterApi(IterApi&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::~IterApi() {}


template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>& IterApi<Set<Key>, Profile>::operator=(const IterApi& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>& IterApi<Set<Key>, Profile>::operator=(IterApi&& other)
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Profile>
bool IterApi<Set<Key>, Profile>::operator==(const IterApi& other) const
{
    return pimpl_ == other.pimpl_;
}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}

template <typename Key, typename Profile>
bool IterApi<Set<Key>, Profile>::is_end() const
{
    return pimpl_->isEnd();
}

template <typename Key, typename Profile>
Key IterApi<Set<Key>, Profile>::key() const
{
    return pimpl_->key();
}



template <typename Key, typename Profile>
bool IterApi<Set<Key>, Profile>::next()
{
    return pimpl_->next();
}

template <typename Key, typename Profile>
bool IterApi<Set<Key>, Profile>::prev()
{
    return pimpl_->prev();
}


template <typename Key, typename Profile>
void IterApi<Set<Key>, Profile>::remove()
{
    return pimpl_->remove();
}

template <typename Key, typename Profile>
void IterApi<Set<Key>, Profile>::insert(const Key& key)
{
    return pimpl_->insert(key);
}

template <typename Key, typename Profile>
BigInt IterApi<Set<Key>, Profile>::for_each(std::function<bool(const Key&)>)
{
    return 0;
}


    
}}
