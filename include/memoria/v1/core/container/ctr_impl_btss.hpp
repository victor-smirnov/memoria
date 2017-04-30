
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/container/container.hpp>

#include "ctr_api_btss.hpp"
#include "ctr_impl.hpp"

#include <memory>

namespace memoria {
namespace v1 {

template <typename CtrName, typename Profile>    
CtrApiBTSSBase<CtrName, Profile>::CtrApiBTSSBase(const std::shared_ptr<AllocatorT>& allocator, int command, const UUID& name):
    pimpl_(std::make_shared<SharedCtr<CtrName, IWalkableAllocator<ProfilePageType<Profile>>, Profile>>(allocator, command, name))
{}

template <typename CtrName, typename Profile>
CtrApiBTSSBase<CtrName, Profile>::~CtrApiBTSSBase() {}

template <typename CtrName, typename Profile>
CtrApiBTSSBase<CtrName, Profile>::CtrApiBTSSBase(const CtrApiBTSSBase& other): pimpl_(other.pimpl_)
{}

template <typename CtrName, typename Profile>
CtrApiBTSSBase<CtrName, Profile>::CtrApiBTSSBase(CtrApiBTSSBase&& other): pimpl_(std::move(other.pimpl_))
{}



template <typename CtrName, typename Profile>
void CtrApiBTSSBase<CtrName, Profile>::operator=(const CtrApiBTSSBase& other) 
{
    pimpl_ = other.pimpl_;
}


template <typename CtrName, typename Profile>
void CtrApiBTSSBase<CtrName, Profile>::operator=(CtrApiBTSSBase&& other) 
{
    pimpl_ = std::move(other.pimpl_);
}



template <typename CtrName, typename Profile>
BigInt CtrApiBTSSBase<CtrName, Profile>::size()
{
    return pimpl_->size();
}


template <typename CtrName, typename Profile>
UUID CtrApiBTSSBase<CtrName, Profile>::name() 
{
    return this->pimpl_->name();
}

template <typename CtrName, typename Profile>
const ContainerMetadataPtr& CtrApiBTSSBase<CtrName, Profile>::metadata() {
    return CtrT::getMetadata();
}


template <typename CtrName, typename Profile>
void CtrApiBTSSBase<CtrName, Profile>::init() {
    CtrT::getMetadata();
}

template <typename CtrName, typename Profile>
void CtrApiBTSSBase<CtrName, Profile>::new_page_size(int size) 
{
    this->pimpl_->setNewPageSize(size);
}


template <typename CtrName, typename Profile>
bool CtrApiBTSSBase<CtrName, Profile>::operator==(const CtrApiBTSSBase& other) const 
{
    return this->pimpl_ == other.pimpl_;
}

template <typename CtrName, typename Profile>
CtrApiBTSSBase<CtrName, Profile>::operator bool() const 
{
    return this->pimpl_ != nullptr;
}




template <typename CtrName, typename Profile>
typename CtrApiBTSSBase<CtrName, Profile>::Iterator CtrApiBTSSBase<CtrName, Profile>::begin() 
{
    return pimpl_->begin();
}


template <typename CtrName, typename Profile>
typename CtrApiBTSSBase<CtrName, Profile>::Iterator CtrApiBTSSBase<CtrName, Profile>::end() 
{
    return pimpl_->end();
}













template <typename CtrName, typename Profile>
IterApiBTSSBase<CtrName, Profile>::IterApiBTSSBase(IterPtr ptr): pimpl_(ptr) {}


template <typename CtrName, typename Profile>
IterApiBTSSBase<CtrName, Profile>::IterApiBTSSBase(const IterApiBTSSBase& other): pimpl_(other.pimpl_) {}

template <typename CtrName, typename Profile>
IterApiBTSSBase<CtrName, Profile>::IterApiBTSSBase(IterApiBTSSBase&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename CtrName, typename Profile>
IterApiBTSSBase<CtrName, Profile>::~IterApiBTSSBase() {}


template <typename CtrName, typename Profile>
void IterApiBTSSBase<CtrName, Profile>::operator=(const IterApiBTSSBase& other) 
{
    pimpl_ = other.pimpl_;
}

template <typename CtrName, typename Profile>
void IterApiBTSSBase<CtrName, Profile>::operator=(IterApiBTSSBase&& other)
{
    pimpl_ = std::move(other.pimpl_);
}


template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::operator==(const IterApiBTSSBase& other) const 
{
    if (pimpl_ && other.pimpl_ && (&pimpl_->ctr() == &other.pimpl_->ctr())) 
    {
        return pimpl_->isEqual(*other.pimpl_.get());
    }
    else if ((!pimpl_) && (!other.pimpl_)) 
    {
        return true;
    }
    
    return false;
}

template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::operator!=(const IterApiBTSSBase& other) const 
{
    return !operator==(other);
}

template <typename CtrName, typename Profile>
IterApiBTSSBase<CtrName, Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}



template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::is_end() const
{
    return this->pimpl_->isEnd();
}


template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::next()
{
    return this->pimpl_->next();
}

template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::prev()
{
    return this->pimpl_->prev();
}


template <typename CtrName, typename Profile>
void IterApiBTSSBase<CtrName, Profile>::remove()
{
    return this->pimpl_->remove();
}


template <typename CtrName, typename Profile>
void IterApiBTSSBase<CtrName, Profile>::dump()
{
    return this->pimpl_->dump();
}


template <typename CtrName, typename Profile>
typename IterApiBTSSBase<CtrName, Profile>::Iterator IterApiBTSSBase<CtrName, Profile>::clone()
{
    return this->pimpl_->clone();
}

template <typename CtrName, typename Profile>
void IterApiBTSSBase<CtrName, Profile>::check(std::ostream& out, const char* source)
{
    return this->pimpl_->check(out, source);
}

template <typename CtrName, typename Profile>
BigInt IterApiBTSSBase<CtrName, Profile>::read(CtrIOBuffer& buffer, BigInt size) 
{
    return this->pimpl_->populate_buffer(&buffer, size);
}

template <typename CtrName, typename Profile>
BigInt IterApiBTSSBase<CtrName, Profile>::read(bt::BufferConsumer<CtrIOBuffer>& consumer, BigInt size) 
{
    return this->pimpl_->read_buffer(&consumer, size);
}

template <typename CtrName, typename Profile>
BigInt IterApiBTSSBase<CtrName, Profile>::read(std::function<Int (CtrIOBuffer&, Int)> consumer, BigInt size) 
{
    BufferFnConsumer<CtrIOBuffer, std::function<Int (CtrIOBuffer&, Int)>> fn_consumer(consumer);
    return this->pimpl_->read_buffer(&fn_consumer, size);
}


template <typename CtrName, typename Profile>
BigInt IterApiBTSSBase<CtrName, Profile>::insert(bt::BufferProducer<CtrIOBuffer>& producer) 
{
    return this->pimpl_->insert_iobuffer(&producer);
}

template <typename CtrName, typename Profile>
BigInt IterApiBTSSBase<CtrName, Profile>::insert(std::function<Int (CtrIOBuffer&)> producer)
{
    BufferFnProducer<CtrIOBuffer, std::function<Int (CtrIOBuffer&)>> fn_producer(producer);
    return this->pimpl_->insert_iobuffer(&fn_producer);
}


}
}
