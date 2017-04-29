
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

#include <memoria/v1/api/map_api.hpp>

#include "map_factory.hpp"

#include <memoria/v1/core/container/ctr_impl.hpp>

#include <memoria/v1/core/tools/static_array.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Key, typename Value, typename Profile>
using SharedMap = SharedCtr<Map<Key, Value>, IWalkableAllocator<ProfilePageType<Profile>>, Profile>;
    
    
template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name):
    pimpl_(std::make_shared<SharedMap<Key, Value, Profile>>(allocator, command, name))
{}

template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::CtrApi(const CtrApi& other):pimpl_(other.pimpl_) {}


template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::CtrApi(CtrApi&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::~CtrApi() {}



template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>& CtrApi<Map<Key, Value>, Profile>::operator=(const CtrApi& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}


template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>& CtrApi<Map<Key, Value>, Profile>::operator=(CtrApi&& other) 
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::operator==(const CtrApi& other) const 
{
    return pimpl_ == other.pimpl_;
}

template <typename Key, typename Value, typename Profile>
CtrApi<Map<Key, Value>, Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}


template <typename Key, typename Value, typename Profile>
BigInt CtrApi<Map<Key, Value>, Profile>::size()
{
    return pimpl_->size();
}


template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::begin() 
{
    return pimpl_->begin();
}


template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::end() 
{
    return pimpl_->end();
}

template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::find(const Key& key) 
{
    return pimpl_->find(key);
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::contains(const Key& key) 
{
    auto iter = pimpl_->find(key);
    
    if (!iter->isEnd()) {
        return iter->key() == key;
    }
    
    return false;
}

template <typename Key, typename Value, typename Profile>
bool CtrApi<Map<Key, Value>, Profile>::remove(const Key& key) 
{
    return pimpl_->remove(key);
}

template <typename Key, typename Value, typename Profile>
typename CtrApi<Map<Key, Value>, Profile>::Iterator CtrApi<Map<Key, Value>, Profile>::assign(const Key& key, const Value& value) 
{
    return pimpl_->assign(key, value);
}

template <typename Key, typename Value, typename Profile>
UUID CtrApi<Map<Key, Value>, Profile>::name() 
{
    return pimpl_->name();
}

template <typename Key, typename Value, typename Profile>
const ContainerMetadataPtr& CtrApi<Map<Key, Value>, Profile>::metadata() {
    return CtrT::getMetadata();
}


template <typename Key, typename Value, typename Profile>
void CtrApi<Map<Key, Value>, Profile>::init() {
    CtrT::getMetadata();
}

template <typename Key, typename Value, typename Profile>
void CtrApi<Map<Key, Value>, Profile>::new_page_size(int size) 
{
    pimpl_->setNewPageSize(size);
}





template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::IterApi(IterPtr ptr): pimpl_(ptr) {}


template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::IterApi(const IterApi& other): pimpl_(other.pimpl_) {}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::IterApi(IterApi&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::~IterApi() {}


template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>& IterApi<Map<Key, Value>, Profile>::operator=(const IterApi& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>& IterApi<Map<Key, Value>, Profile>::operator=(IterApi&& other)
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Key, typename Value, typename Profile>
bool IterApi<Map<Key, Value>, Profile>::operator==(const IterApi& other) const
{
    return pimpl_ == other.pimpl_;
}

template <typename Key, typename Value, typename Profile>
IterApi<Map<Key, Value>, Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}

template <typename Key, typename Value, typename Profile>
bool IterApi<Map<Key, Value>, Profile>::is_end() const
{
    return pimpl_->isEnd();
}

template <typename Key, typename Value, typename Profile>
Key IterApi<Map<Key, Value>, Profile>::key() const
{
    return pimpl_->key();
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
    return detail00::ValueHelper<decltype(pimpl_->value())>::convert(pimpl_->value());
}

template <typename Key, typename Value, typename Profile>
bool IterApi<Map<Key, Value>, Profile>::next()
{
    return pimpl_->next();
}

template <typename Key, typename Value, typename Profile>
bool IterApi<Map<Key, Value>, Profile>::prev()
{
    return pimpl_->prev();
}


template <typename Key, typename Value, typename Profile>
void IterApi<Map<Key, Value>, Profile>::remove()
{
    return pimpl_->remove();
}

template <typename Key, typename Value, typename Profile>
void IterApi<Map<Key, Value>, Profile>::insert(const Key& key, const Value& value)
{
    return pimpl_->insert(key, value);
}


template <typename Key, typename Value, typename Profile>
void IterApi<Map<Key, Value>, Profile>::dump()
{
    return pimpl_->dump();
}


template <typename Key, typename Value, typename Profile>
BigInt IterApi<Map<Key, Value>, Profile>::read(CtrIOBuffer& buffer, BigInt size)
{
    return pimpl_->populate_buffer(&buffer, size);
}

template <typename Key, typename Value, typename Profile>
BigInt IterApi<Map<Key, Value>, Profile>::read(bt::BufferConsumer<CtrIOBuffer>& consumer, BigInt size)
{
    return pimpl_->read_buffer(&consumer, size);
}

template <typename Key, typename Value, typename Profile>
BigInt IterApi<Map<Key, Value>, Profile>::read(std::function<Int (CtrIOBuffer&, Int)> consumer, BigInt size) 
{
    BufferFnConsumer<CtrIOBuffer, std::function<Int (CtrIOBuffer&, Int)>> fn_consumer(consumer);
    return pimpl_->read_buffer(&fn_consumer, size);
}

template <typename Key, typename Value, typename Profile>
BigInt IterApi<Map<Key, Value>, Profile>::insert(bt::BufferProducer<CtrIOBuffer>& producer, BigInt size)
{
    return pimpl_->insert_iobuffer(&producer);
}

template <typename Key, typename Value, typename Profile>
BigInt IterApi<Map<Key, Value>, Profile>::insert(std::function<Int (CtrIOBuffer&)> producer) 
{
    BufferFnProducer<CtrIOBuffer, std::function<Int (CtrIOBuffer&)>> fn_producer(producer);
    return pimpl_->insert_iobuffer(&fn_producer);
}



    
}}
