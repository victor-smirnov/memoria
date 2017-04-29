
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

#include "set_factory.hpp"

#include <memoria/v1/api/set_api.hpp>
#include <memoria/v1/core/container/ctr_impl_btss.hpp>

#include <memory>

namespace memoria {
namespace v1 {


    
template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name):
    Base(allocator, command, name)
{}

template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::CtrApi(const CtrApi& other):Base(other) {}


template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::CtrApi(CtrApi&& other): Base(std::move(other)) {}

template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::~CtrApi() {}



template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>& CtrApi<Set<Key>, Profile>::operator=(const CtrApi& other) 
{
    Base::pimpl_ = other.pimpl_;
    return *this;
}


template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>& CtrApi<Set<Key>, Profile>::operator=(CtrApi&& other) 
{
    Base::pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::operator==(const CtrApi& other) const 
{
    return Base::pimpl_ == other.pimpl_;
}

template <typename Key, typename Profile>
CtrApi<Set<Key>, Profile>::operator bool() const 
{
    return Base::pimpl_ != nullptr;
}




template <typename Key, typename Profile>
typename CtrApi<Set<Key>, Profile>::Iterator CtrApi<Set<Key>, Profile>::begin() 
{
    return Base::pimpl_->begin();
}


template <typename Key, typename Profile>
typename CtrApi<Set<Key>, Profile>::Iterator CtrApi<Set<Key>, Profile>::end() 
{
    return Base::pimpl_->end();
}

template <typename Key, typename Profile>
typename CtrApi<Set<Key>, Profile>::Iterator CtrApi<Set<Key>, Profile>::find(const Key& key) 
{
    return Base::pimpl_->find(key);
}

template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::contains(const Key& key) 
{
    return Base::pimpl_->contains(key);
}

template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::remove(const Key& key) 
{
    return Base::pimpl_->remove(key);
}

template <typename Key, typename Profile>
bool CtrApi<Set<Key>, Profile>::insert(const Key& key) 
{
    return Base::pimpl_->insert_key(key);
}




template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::IterApi(IterPtr ptr): Base(ptr) {}


template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::IterApi(const IterApi& other): Base(other) {}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::IterApi(IterApi&& other): Base(std::move(other)) {}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::~IterApi() {}


template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>& IterApi<Set<Key>, Profile>::operator=(const IterApi& other) 
{
    this->pimpl_ = other.pimpl_;
    return *this;
}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>& IterApi<Set<Key>, Profile>::operator=(IterApi&& other)
{
    this->pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Profile>
bool IterApi<Set<Key>, Profile>::operator==(const IterApi& other) const
{
    return this->pimpl_ == other.pimpl_;
}

template <typename Key, typename Profile>
IterApi<Set<Key>, Profile>::operator bool() const 
{
    return this->pimpl_ != nullptr;
}


template <typename Key, typename Profile>
Key IterApi<Set<Key>, Profile>::key() const
{
    return this->pimpl_->key();
}




template <typename Key, typename Profile>
void IterApi<Set<Key>, Profile>::insert(const Key& key)
{
    return this->pimpl_->insert(key);
}





    
}}
