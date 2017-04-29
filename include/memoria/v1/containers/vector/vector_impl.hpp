
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

#include "vctr_factory.hpp"

#include <memoria/v1/api/vector_api.hpp>
#include <memoria/v1/core/container/ctr_impl_btss.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Value, typename Profile>
using SharedVector = SharedCtr<Vector<Value>, IWalkableAllocator<ProfilePageType<Profile>>, Profile>;

template <typename Value, typename Profile>
CtrApi<Vector<Value>, Profile>::CtrApi(const std::shared_ptr<AllocatorT>& allocator, Int command, const UUID& name):
    pimpl_(std::make_shared<SharedVector<Value, Profile>>(allocator, command, name))
{}

template <typename Value, typename Profile>
CtrApi<Vector<Value>, Profile>::~CtrApi() {}



template <typename Value, typename Profile>
CtrApi<Vector<Value>, Profile>::CtrApi(const CtrApi& other): pimpl_(other.pimpl_)
{}

template <typename Value, typename Profile>
CtrApi<Vector<Value>, Profile>::CtrApi(CtrApi&& other): pimpl_(std::move(other.pimpl_))
{}

template <typename Value, typename Profile>
CtrApi<Vector<Value>, Profile>& CtrApi<Vector<Value>, Profile>::operator=(const CtrApi& other)
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Value, typename Profile>
CtrApi<Vector<Value>, Profile>& CtrApi<Vector<Value>, Profile>::operator=(CtrApi&& other)
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Value, typename Profile>
bool CtrApi<Vector<Value>, Profile>::operator==(const CtrApi& other) const
{
    return pimpl_ == other.pimpl_;
}

template <typename Value, typename Profile>
CtrApi<Vector<Value>, Profile>::operator bool() const
{
    return pimpl_ != nullptr;
}

template <typename Value, typename Profile>
UUID CtrApi<Vector<Value>, Profile>::name()
{
    return pimpl_->name();
}

template <typename Value, typename Profile>
BigInt CtrApi<Vector<Value>, Profile>::size()
{
    return pimpl_->size();
}

template <typename Value, typename Profile>
typename CtrApi<Vector<Value>, Profile>::Iterator CtrApi<Vector<Value>, Profile>::begin()
{
    return pimpl_->begin();
}

template <typename Value, typename Profile>
typename CtrApi<Vector<Value>, Profile>::Iterator CtrApi<Vector<Value>, Profile>::end()
{
    return pimpl_->end();
}

template <typename Value, typename Profile>
typename CtrApi<Vector<Value>, Profile>::Iterator CtrApi<Vector<Value>, Profile>::seek(BigInt pos)
{
    return pimpl_->seek(pos);
}

template <typename Value, typename Profile>
void CtrApi<Vector<Value>, Profile>::init()
{
    CtrT::getMetadata();
}

template <typename Value, typename Profile>
const ContainerMetadataPtr& CtrApi<Vector<Value>, Profile>::metadata()
{
    return CtrT::getMetadata();
}



template <typename Value, typename Profile>
IterApi<Vector<Value>, Profile>::IterApi(IterPtr ptr): pimpl_(ptr) {}

template <typename Value, typename Profile>
IterApi<Vector<Value>, Profile>::IterApi(const IterApi& other): pimpl_(other.pimpl_) {}

template <typename Value, typename Profile>
IterApi<Vector<Value>, Profile>::IterApi(IterApi&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Value, typename Profile>
IterApi<Vector<Value>, Profile>::~IterApi() {}

template <typename Value, typename Profile>
IterApi<Vector<Value>, Profile>& IterApi<Vector<Value>, Profile>::operator=(const IterApi& other)
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Value, typename Profile>
IterApi<Vector<Value>, Profile>& IterApi<Vector<Value>, Profile>::operator=(IterApi&& other)
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}

template <typename Value, typename Profile>
bool IterApi<Vector<Value>, Profile>::operator==(const IterApi& other) const
{
    return pimpl_ == other.pimpl_;
}

template <typename Value, typename Profile>
IterApi<Vector<Value>, Profile>::operator bool() const
{
    return pimpl_ != nullptr;
}

/*
bool is_end() const;

bool next();
bool prev();

Value value() const;
void insert(const Value& value);
void remove();

void dump();

BigInt read(CtrIOBuffer& buffer, BigInt size);
BigInt read(bt::BufferConsumer<CtrIOBuffer>& consumer, BigInt size);

BigInt read(std::function<Int (CtrIOBuffer&, Int)> consumer, BigInt);

BigInt insert(bt::BufferProducer<CtrIOBuffer>& producer);
BigInt insert(std::function<Int (CtrIOBuffer&)> producer);

*/
    
}}
