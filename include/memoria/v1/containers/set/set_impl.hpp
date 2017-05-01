
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

#include <memoria/v1/api/set/set_api.hpp>
#include <memoria/v1/core/container/ctr_impl_btss.hpp>

#include <memory>

namespace memoria {
namespace v1 {


 





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
