
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/api/common/ctr_api.hpp>

#include <memory>

namespace memoria {
namespace v1 {
    
    
    

template <typename CtrName, typename Allocator, typename Profile>
class SharedCtr: public CtrTF<Profile, CtrName, CtrName>::Type {
    using Base = typename CtrTF<Profile, CtrName, CtrName>::Type;
public:
    SharedCtr(const CtrSharedPtr<Allocator>& allocator, int32_t command, const UUID& name):
        Base(allocator, command, name)
    {}
    
    SharedCtr(const CtrSharedPtr<Allocator>& allocator, const UUID& root_id, const CtrInitData& ctr_init_data):
        Base(allocator, root_id, ctr_init_data)
    {}
};


template <typename CtrName, typename Profile>
class SharedIter: public Iter<typename CtrTF<Profile, CtrName, CtrName>::Types::IterTypes> {
    
    using MyType1 = SharedIter<CtrName,Profile>;
    
    using CtrT = typename CtrTF<Profile, CtrName, CtrName>::Type;
    using IterT = Iter<typename CtrTF<Profile, CtrName, CtrName>::Types::IterTypes>;

    using CtrPtr = CtrSharedPtr<CtrT>;
    
    using Base = Iter<typename CtrTF<Profile, CtrName, CtrName>::Types::IterTypes>;
public:
    SharedIter(): Base(){}

    SharedIter(CtrPtr ptr): Base(std::move(ptr))
    {
        Base::idx() = 0;
    }
    
    SharedIter(const MyType1& other): Base(other) {}
    SharedIter(const IterT& other): Base(other) {}
};







template <typename CtrName, typename Profile>    
CtrApiBase<CtrName, Profile>::CtrApiBase(const CtrSharedPtr<AllocatorT>& allocator, int command, const UUID& name):
    pimpl_(ctr_make_shared<SharedCtr<CtrName, IAllocator<ProfilePageType<Profile>>, Profile>>(allocator, command, name))
{}

template <typename CtrName, typename Profile>    
CtrApiBase<CtrName, Profile>::CtrApiBase(CtrPtr ptr):pimpl_(std::move(ptr))
{}

template <typename CtrName, typename Profile>    
CtrApiBase<CtrName, Profile>::CtrApiBase(){}


template <typename CtrName, typename Profile>
CtrApiBase<CtrName, Profile>::~CtrApiBase() {}

template <typename CtrName, typename Profile>
CtrApiBase<CtrName, Profile>::CtrApiBase(const CtrApiBase& other): pimpl_(other.pimpl_)
{}

template <typename CtrName, typename Profile>
CtrApiBase<CtrName, Profile>::CtrApiBase(CtrApiBase&& other): pimpl_(std::move(other.pimpl_))
{}

template <typename CtrName, typename Profile>
typename CtrApiBase<CtrName, Profile>::CtrPtr& CtrApiBase<CtrName, Profile>::ptr() {
    return pimpl_;
}

template <typename CtrName, typename Profile>
const typename CtrApiBase<CtrName, Profile>::CtrPtr& CtrApiBase<CtrName, Profile>::ptr() const {
    return pimpl_;
}


template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::operator=(const CtrApiBase& other) 
{
    pimpl_ = other.pimpl_;
}


template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::operator=(CtrApiBase&& other) 
{
    pimpl_ = std::move(other.pimpl_);
}





template <typename CtrName, typename Profile>
UUID CtrApiBase<CtrName, Profile>::name() 
{
    return this->pimpl_->name();
}

template <typename CtrName, typename Profile>
const ContainerMetadataPtr& CtrApiBase<CtrName, Profile>::metadata() {
    return CtrT::getMetadata();
}


template <typename CtrName, typename Profile>
ContainerMetadataPtr CtrApiBase<CtrName, Profile>::init() {
    return CtrT::getMetadata();
}

template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::do_link() {}

template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::new_page_size(int size) 
{
    this->pimpl_->setNewPageSize(size);
}

template <typename CtrName, typename Profile>
int CtrApiBase<CtrName, Profile>::new_page_size()
{
    return this->pimpl_->getNewPageSize();
}

template <typename CtrName, typename Profile>
bool CtrApiBase<CtrName, Profile>::operator==(const CtrApiBase& other) const 
{
    return this->pimpl_ == other.pimpl_;
}

template <typename CtrName, typename Profile>
CtrApiBase<CtrName, Profile>::operator bool() const 
{
    return this->pimpl_ != nullptr;
}

template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::reset() 
{
    this->pimpl_.reset();
}

template <typename CtrName, typename Profile>
CtrRef<Profile> CtrApiBase<CtrName, Profile>::to_ref() 
{
    return static_pointer_cast<CtrReferenceable>(pimpl_);
}

template <typename CtrName, typename Profile>
CtrApi<CtrName, Profile> CtrApiBase<CtrName, Profile>::cast_from(const CtrRef<Profile>& ref)
{
    return static_pointer_cast<CtrT>(ref.ptr_);
}


template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::drop()
{
    this->pimpl_->drop();
}

template <typename CtrName, typename Profile>
UUID CtrApiBase<CtrName, Profile>::clone_ctr(const UUID& new_name)
{
    return this->pimpl_->clone(new_name);
}

template <typename CtrName, typename Profile>
UUID CtrApiBase<CtrName, Profile>::clone_ctr()
{
    return this->pimpl_->clone(UUID{});
}


template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::cleanup()
{
    this->pimpl_->cleanup();
}

template <typename CtrName, typename Profile>
int32_t CtrApiBase<CtrName, Profile>::metadata_links_num() const
{
    return this->pimpl_->metadata_links_num();
}


template <typename CtrName, typename Profile>
UUID CtrApiBase<CtrName, Profile>::get_metadata_link(int num) const
{
    return this->pimpl_->get_metadata_link(num);
}


template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::set_metadata_link(int num, const UUID& link_id)
{
    return this->pimpl_->set_metadata_link(num, link_id);
}


template <typename CtrName, typename Profile>
std::string CtrApiBase<CtrName, Profile>::get_descriptor_str() const
{
    return this->pimpl_->get_descriptor_str();
}


template <typename CtrName, typename Profile>
void CtrApiBase<CtrName, Profile>::set_descriptor_str(const std::string& str)
{
    return this->pimpl_->set_descriptor_str(str);
}


template <typename CtrName, typename Profile>
bool CtrApiBase<CtrName, Profile>::is_updatable() const
{
    return this->pimpl_->is_updatable();
}


template <typename CtrName, typename Profile>
PairPtr& CtrApiBase<CtrName, Profile>::pair()
{
    return this->pimpl_->pair();
}

template <typename CtrName, typename Profile>
const PairPtr& CtrApiBase<CtrName, Profile>::pair() const
{
    return this->pimpl_->pair();
}



template <typename CtrName, typename Profile>
IterApiBase<CtrName, Profile>::IterApiBase(IterPtr ptr): pimpl_(ptr) {}

template <typename CtrName, typename Profile>
IterApiBase<CtrName, Profile>::IterApiBase() {}


template <typename CtrName, typename Profile>
IterApiBase<CtrName, Profile>::IterApiBase(const IterApiBase& other): pimpl_(other.pimpl_) {}

template <typename CtrName, typename Profile>
IterApiBase<CtrName, Profile>::IterApiBase(IterApiBase&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename CtrName, typename Profile>
IterApiBase<CtrName, Profile>::~IterApiBase() {}

template <typename CtrName, typename Profile>
const typename IterApiBase<CtrName, Profile>::IterPtr& IterApiBase<CtrName, Profile>::ptr() const {
    return pimpl_;
}

template <typename CtrName, typename Profile>
typename IterApiBase<CtrName, Profile>::IterPtr& IterApiBase<CtrName, Profile>::ptr() {
    return pimpl_;
}

template <typename CtrName, typename Profile>
void IterApiBase<CtrName, Profile>::operator=(const IterApiBase& other) 
{
    pimpl_ = other.pimpl_;
}

template <typename CtrName, typename Profile>
void IterApiBase<CtrName, Profile>::operator=(IterApiBase&& other)
{
    pimpl_ = std::move(other.pimpl_);
}


template <typename CtrName, typename Profile>
bool IterApiBase<CtrName, Profile>::operator==(const IterApiBase& other) const 
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
bool IterApiBase<CtrName, Profile>::operator!=(const IterApiBase& other) const 
{
    return !operator==(other);
}

template <typename CtrName, typename Profile>
IterApiBase<CtrName, Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}

template <typename CtrName, typename Profile>
CtrApi<CtrName, Profile> IterApiBase<CtrName, Profile>::ctr() 
{
    return static_pointer_cast<CtrT>(pimpl_->ctr_ptr());
}




template <typename CtrName, typename Profile>
void IterApiBase<CtrName, Profile>::dump()
{
    return this->pimpl_->dump();
}

template <typename CtrName, typename Profile>
void IterApiBase<CtrName, Profile>::dump_path()
{
    return this->pimpl_->dumpPath();
}

template <typename CtrName, typename Profile>
void IterApiBase<CtrName, Profile>::dump_header()
{
    return this->pimpl_->dumpHeader();
}

template <typename CtrName, typename Profile>
void IterApiBase<CtrName, Profile>::reset()
{
    return this->pimpl_.reset();
}



template <typename CtrName, typename Profile>
typename IterApiBase<CtrName, Profile>::Iterator IterApiBase<CtrName, Profile>::clone()
{
    return this->pimpl_->clone();
}

template <typename CtrName, typename Profile>
void IterApiBase<CtrName, Profile>::check(std::ostream& out, const char* source)
{
    return this->pimpl_->check(out, source);
}


template <typename CtrName, typename Profile>
PairPtr& IterApiBase<CtrName, Profile>::pair()
{
    return this->pimpl_->pair();
}

template <typename CtrName, typename Profile>
const PairPtr& IterApiBase<CtrName, Profile>::pair() const
{
    return this->pimpl_->pair();
}


}
}
