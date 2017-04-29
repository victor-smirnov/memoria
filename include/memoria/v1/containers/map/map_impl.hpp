
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

#include "map_factory.hpp"

#include <memoria/v1/api/map_api.hpp>
#include <memoria/v1/core/container/ctr_impl_btss.hpp>
#include <memoria/v1/core/tools/static_array.hpp>



#include <memory>

namespace memoria {
namespace v1 {

// template <typename Key, typename Value, typename Profile>
// using SharedMap = SharedCtr<Map<Key, Value>, IWalkableAllocator<ProfilePageType<Profile>>, Profile>;
    
    
template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name):
    Base(allocator, command, name)
{}

template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::CtrApi(const CtrApi& other):Base(other) {}


template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::CtrApi(CtrApi&& other): Base(std::move(other)) {}

template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::~CtrApi() {}



template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>& CtrApi<Map<Key, Value>, Profile>::operator=(const CtrApi& other) 
{
    this->pimpl_ = other.pimpl_;
    return *this;
}


template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>& CtrApi<Map<Key, Value>, Profile>::operator=(CtrApi&& other) 
{
    this->pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::operator==(const CtrApi& other) const 
{
    return this->pimpl_ == other.pimpl_;
}

template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::operator bool() const 
{
    return this->pimpl_ != nullptr;
}





template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::begin() 
{
    return this->pimpl_->begin();
}


template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::end() 
{
    return this->pimpl_->end();
}

template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::find(const Key& key) 
{
    return this->pimpl_->find(key);
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::contains(const Key& key) 
{
    auto iter = this->pimpl_->find(key);
    
    if (!iter->isEnd()) {
        return iter->key() == key;
    }
    
    return false;
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::remove(const Key& key) 
{
    return this->pimpl_->remove(key);
}

template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::assign(const Key& key, const Value& value) 
{
    return this->pimpl_->assign(key, value);
}







template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::IterApi(IterPtr ptr): Base(ptr) {}


template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::IterApi(const IterApi& other): Base(other) {}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::IterApi(IterApi&& other): Base(std::move(other)) {}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::~IterApi() {}


template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>& IterApi<Map<Key, Value>, Profile>::operator=(const IterApi& other) 
{
    this->pimpl_ = other.pimpl_;
    return *this;
}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>& IterApi<Map<Key, Value>, Profile>::operator=(IterApi&& other)
{
    this->pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Value, typename Profile>
bool IterApi<Map<Key, Value>, Profile>::operator==(const IterApi& other) const
{
    return this->pimpl_ == other.pimpl_;
}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::operator bool() const 
{
    return this->pimpl_ != nullptr;
}


template <typename Key, typename Value, typename Profile>
Key IterApi<Map<Key, Value>, Profile>::key() const
{
    return this->pimpl_->key();
}


namespace detail00 {
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
Value IterApi<Map<Key, Value>, Profile>::value() const
{
    return detail00::ValueHelper<decltype(this->pimpl_->value())>::convert(this->pimpl_->value());
}



template <typename Key, typename Value, typename Profile>
void IterApi<Map<Key, Value>, Profile>::insert(const Key& key, const Value& value)
{
    return this->pimpl_->insert(key, value);
}








    
}}
