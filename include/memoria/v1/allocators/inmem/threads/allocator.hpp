
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/latch.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include "../common/allocator_base.hpp"
#include "persistent_tree_snapshot.hpp"


#include <malloc.h>
#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>



namespace memoria {
namespace v1 {

namespace persistent_inmem {

template <typename Profile>
class ThreadInMemAllocatorImpl: public InMemAllocatorBase<Profile, ThreadInMemAllocatorImpl<Profile>> { 
public:
    using Base      = InMemAllocatorBase<Profile, ThreadInMemAllocatorImpl<Profile>>;
    using MyType    = ThreadInMemAllocatorImpl<Profile>;
    
    using typename Base::Page;

    
    using typename Base::HistoryNode;
    using typename Base::HistoryNodeBuffer;
    
    using Base::load;
    using Base::store;
    
    template <typename, typename>
    friend class InMemAllocatorBase;

private:
 
public:
    ThreadInMemAllocatorImpl() {}

private:
    ThreadInMemAllocatorImpl(int32_t v): Base(v) {}


public:

    virtual ~ThreadInMemAllocatorImpl()
    {

    }


    virtual void store(const char* file)
    {
    	auto fileh = FileOutputStreamHandler::create(file);
        Base::store(fileh.get());
    }

    void dump(const char* path)
    {
        using Walker = FSDumpContainerWalker<Page>;

        Walker walker(this->getMetadata(), path);
        this->walkContainers(&walker);
    }
 
    static AllocSharedPtr<MyType> load(const char* file)
    {
        auto fileh = FileInputStreamHandler::create(file);
        return Base::load(fileh.get());
    }

private:


};

}

template <typename Profile = DefaultProfile<>>
using PersistentInMemAllocator = class persistent_inmem::ThreadInMemAllocatorImpl<Profile>;



template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator() {}

template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator(AllocSharedPtr<PImpl> impl): pimpl_(impl) {}

template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator(ThreadInMemAllocator&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Profile>
ThreadInMemAllocator<Profile>::ThreadInMemAllocator(const ThreadInMemAllocator& other): pimpl_(other.pimpl_) {}

template <typename Profile>
ThreadInMemAllocator<Profile>& ThreadInMemAllocator<Profile>::operator=(const ThreadInMemAllocator& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Profile>
ThreadInMemAllocator<Profile>& ThreadInMemAllocator<Profile>::operator=(ThreadInMemAllocator&& other) 
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}

template <typename Profile>
ThreadInMemAllocator<Profile>::~ThreadInMemAllocator() {}


template <typename Profile>
bool ThreadInMemAllocator<Profile>::operator==(const ThreadInMemAllocator& other) const
{
    return pimpl_ == other.pimpl_;
}


template <typename Profile>
ThreadInMemAllocator<Profile>::operator bool() const 
{
    return pimpl_ != nullptr;
}



template <typename Profile>
ThreadInMemAllocator<Profile> ThreadInMemAllocator<Profile>::load(InputStreamHandler* input_stream) 
{
    return ThreadInMemAllocator<Profile>(PImpl::load(input_stream));
}

template <typename Profile>
ThreadInMemAllocator<Profile> ThreadInMemAllocator<Profile>::load(boost::filesystem::path file_name) 
{
    return ThreadInMemAllocator<Profile>(PImpl::load(file_name.string().c_str()));
}

template <typename Profile>
ThreadInMemAllocator<Profile> ThreadInMemAllocator<Profile>::create() 
{
    return ThreadInMemAllocator<Profile>(PImpl::create());
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::store(boost::filesystem::path file_name) 
{
    pimpl_->store(file_name.string().c_str());
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::store(OutputStreamHandler* output_stream) 
{
    pimpl_->store(output_stream);
}

template <typename Profile>
typename ThreadInMemAllocator<Profile>::SnapshotPtr ThreadInMemAllocator<Profile>::master() 
{
    return SnapshotPtr(pimpl_->master());
}


template <typename Profile>
typename ThreadInMemAllocator<Profile>::SnapshotPtr ThreadInMemAllocator<Profile>::find(const TxnId& snapshot_id) 
{
    return pimpl_->find(snapshot_id);
}

template <typename Profile>
typename ThreadInMemAllocator<Profile>::SnapshotPtr ThreadInMemAllocator<Profile>::find_branch(StringRef name) 
{
    return pimpl_->find_branch(name);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::set_master(const TxnId& txn_id) 
{
    pimpl_->set_master(txn_id);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::set_branch(StringRef name, const TxnId& txn_id) 
{
    pimpl_->set_branch(name, txn_id);
}

template <typename Profile>
ContainerMetadataRepository* ThreadInMemAllocator<Profile>::metadata() const 
{
    return pimpl_->getMetadata();
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::walk_containers(ContainerWalker* walker, const char* allocator_descr) 
{
     return pimpl_->walkContainers(walker, allocator_descr);
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::dump(boost::filesystem::path dump_at) 
{
    pimpl_->dump(dump_at.string().c_str());
}

template <typename Profile>
void ThreadInMemAllocator<Profile>::reset() 
{
    pimpl_.reset();
}


template <typename Profile>
void ThreadInMemAllocator<Profile>::pack() 
{
    return pimpl_->pack();
}

template <typename Profile>
Logger& ThreadInMemAllocator<Profile>::logger()
{
    return pimpl_->logger();
}


template <typename Profile>
bool ThreadInMemAllocator<Profile>::check() 
{
    return pimpl_->check();
}

}}
