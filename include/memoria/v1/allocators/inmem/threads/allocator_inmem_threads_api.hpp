
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
#include <memoria/v1/core/tools/strings/strings.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/metadata_repository.hpp>

#include <boost/filesystem.hpp>

#include <memory>

namespace memoria {
namespace v1 {
    

namespace persistent_inmem_thread {
    template <typename Profile> class ThreadInMemAllocatorImpl;
    template <typename Profile, typename PersistentAllocator> class Snapshot;
}

template <typename Profile = DefaultProfile<>>
class ThreadInMemSnapshot;
	
template <typename Profile = DefaultProfile<>>
class ThreadInMemAllocator {
    using PImpl = persistent_inmem_thread::ThreadInMemAllocatorImpl<Profile>;
    
    using SnapshotPtr = ThreadInMemSnapshot<Profile>;
    using TxnId = UUID;
    
    std::shared_ptr<PImpl> pimpl_;
public:
    ThreadInMemAllocator(std::shared_ptr<PImpl> impl);
    ThreadInMemAllocator(ThreadInMemAllocator&& impl);
    
    ThreadInMemAllocator(const ThreadInMemAllocator&);
    ThreadInMemAllocator& operator=(const ThreadInMemAllocator&);
    
    ~ThreadInMemAllocator();
    
    ThreadInMemAllocator& operator=(ThreadInMemAllocator&&);
    
    static ThreadInMemAllocator load(InputStreamHandler* input_stream);
    static ThreadInMemAllocator load(boost::filesystem::path file_name);
    
    static ThreadInMemAllocator create();
    
    void store(boost::filesystem::path file_name);
    void store(OutputStreamHandler* output_stream);
    
    SnapshotPtr master();
    SnapshotPtr find(const TxnId& snapshot_id);
    SnapshotPtr find_branch(StringRef name);
    
    void set_master(const TxnId& txn_id);
    void set_branch(StringRef name, const TxnId& txn_id);
    
    ContainerMetadataRepository* getMetadata() const;
    
    void dump(boost::filesystem::path dump_at);
};


template <typename CtrT, typename Allocator>
class SharedCtr: public CtrT {

public:
    SharedCtr(const std::shared_ptr<Allocator>& allocator, Int command, const UUID& name):
        CtrT(allocator.get(), command, name)
    {
        CtrT::alloc_holder_ = allocator;
    }

    auto snapshot() const {
        return CtrT::alloc_holder_;
    }
};


template <typename Profile>
class ThreadInMemSnapshot {
    using AllocatorImpl = persistent_inmem_thread::ThreadInMemAllocatorImpl<Profile>;
    using PImpl         = persistent_inmem_thread::Snapshot<Profile, AllocatorImpl>;
    
    using SnapshotPtr = ThreadInMemSnapshot;
    using TxnId = UUID;

public:
    template <typename CtrName>
    using CtrT = SharedCtr<typename CtrTF<Profile, CtrName>::Type, IWalkableAllocator<ProfilePageType<Profile>>>;
    
    std::shared_ptr<PImpl> pimpl_;
public:
    ThreadInMemSnapshot(std::shared_ptr<PImpl> impl);
    ThreadInMemSnapshot(ThreadInMemSnapshot&& impl);
    
    ThreadInMemSnapshot(const ThreadInMemSnapshot&);
    ThreadInMemSnapshot& operator=(const ThreadInMemSnapshot&);
    
    ~ThreadInMemSnapshot();
    
    ThreadInMemSnapshot& operator=(ThreadInMemSnapshot&&);
    
    ContainerMetadataRepository* getMetadata() const;
    const UUID& uuid() const;
    bool is_active() const;
    bool is_marked_to_clear() const;
    bool is_committed() const;
    void commit();
    void drop();
    bool drop_ctr(const UUID& name);
    void set_as_master();
    void set_as_branch(StringRef name);
    StringRef metadata() const;
    void set_metadata(StringRef metadata);
    void lock_data_for_import();
    SnapshotPtr branch();
    bool has_parent() const;
    SnapshotPtr parent();
    void import_new_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    void copy_new_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    void import_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    void copy_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    bool check();
    void dump(boost::filesystem::path destination);
    void dump_persistent_tree();
    
    std::shared_ptr<IWalkableAllocator<ProfilePageType<Profile>>> snapshot_ref();
    
    template <typename CtrName>
    auto find_or_create(const UUID& name)
    {
    	//pimpl_->checkIfConainersCreationAllowed();
        return std::make_shared<CtrT<CtrName>>(snapshot_ref(), CTR_FIND | CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create(const UUID& name)
    {
    	//pimpl_->checkIfConainersCreationAllowed();
        return std::make_shared<CtrT<CtrName>>(snapshot_ref(), CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create()
    {
    	//pimpl_->checkIfConainersCreationAllowed();
        return std::make_shared<CtrT<CtrName>>(snapshot_ref(), CTR_CREATE, CTR_DEFAULT_NAME);
    }

    template <typename CtrName>
    auto find(const UUID& name)
    {
    	//pimpl_->checkIfConainersOpeneingAllowed();
        return std::make_shared<CtrT<CtrName>>(snapshot_ref(), CTR_FIND, name);
    }
};




template <typename CtrName, typename Profile>
auto create(ThreadInMemSnapshot<Profile>& alloc, const UUID& name)
{
    return alloc.template create<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto create(ThreadInMemSnapshot<Profile>&& alloc, const UUID& name)
{
    return alloc.template create<CtrName>(name);
}



template <typename CtrName, typename Profile>
auto create(ThreadInMemSnapshot<Profile>& alloc)
{
    
    //using CtrT = typename ThreadInMemSnapshot<Profile>::template CtrT<CtrName>;
    
    //return std::make_shared<CtrT>(alloc.snapshot_ref(), CTR_CREATE, CTR_DEFAULT_NAME);
    
    return alloc.template create<CtrName>();
}

template <typename CtrName, typename Profile>
auto create(ThreadInMemSnapshot<Profile>&& alloc)
{
    return alloc.template create<CtrName>();
}



template <typename CtrName, typename Profile>
auto find_or_create(ThreadInMemSnapshot<Profile>& alloc, const UUID& name)
{
    return alloc.template find_or_create<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto find_or_create(ThreadInMemSnapshot<Profile>&& alloc, const UUID& name)
{
    return alloc.template find_or_create<CtrName>(name);
}



template <typename CtrName, typename Profile>
auto find(ThreadInMemSnapshot<Profile>& alloc, const UUID& name)
{
    return alloc.template find<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto find(ThreadInMemSnapshot<Profile>&& alloc, const UUID& name)
{
    return alloc.template find<CtrName>(name);
}



}
}
