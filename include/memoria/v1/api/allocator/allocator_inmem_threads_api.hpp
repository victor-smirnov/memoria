
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
#include <memoria/v1/core/strings/strings.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/api/common/ctr_api.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/logs.hpp>

#include <memoria/v1/allocators/inmem/common/container_collection_cfg.hpp>

#include <boost/filesystem.hpp>

#include "allocator_inmem_api_common.hpp"

namespace memoria {
namespace v1 {


namespace persistent_inmem {
    template <typename Profile> class ThreadInMemAllocatorImpl;
    template <typename Profile, typename PersistentAllocator> class ThreadSnapshot;
}

template <typename Profile = DefaultProfile<>>
class ThreadInMemSnapshot;

template <typename Profile = DefaultProfile<>>
class ThreadInMemAllocator {
    using PImpl = persistent_inmem::ThreadInMemAllocatorImpl<Profile>;
    using TxnId = UUID;
    
    AllocSharedPtr<PImpl> pimpl_;
public:
    using SnapshotPtr   = ThreadInMemSnapshot<Profile>;
    using Page          = ProfilePageType<Profile>;
    
    ThreadInMemAllocator();
    
    ThreadInMemAllocator(AllocSharedPtr<PImpl> impl);
    ThreadInMemAllocator(ThreadInMemAllocator&& impl);
    
    ThreadInMemAllocator(const ThreadInMemAllocator&);
    ThreadInMemAllocator& operator=(const ThreadInMemAllocator&);
    
    ~ThreadInMemAllocator();
    
    ThreadInMemAllocator& operator=(ThreadInMemAllocator&&);
    
    bool operator==(const ThreadInMemAllocator&) const;
    operator bool() const;
    
    
    static ThreadInMemAllocator load(InputStreamHandler* input_stream);
    static ThreadInMemAllocator load(boost::filesystem::path file_name);
    
    static ThreadInMemAllocator create();
    
    void store(boost::filesystem::path file_name);
    void store(OutputStreamHandler* output_stream);
    
    SnapshotPtr master();
    SnapshotPtr find(const TxnId& snapshot_id);
    SnapshotPtr find_branch(U16StringRef name);
    
    void set_master(const TxnId& txn_id);
    void set_branch(U16StringRef name, const TxnId& txn_id);
    
    ContainerMetadataRepository* metadata() const;
    void walk_containers(ContainerWalker* walker, const char16_t* allocator_descr = nullptr);
    
    Logger& logger();
    
    void reset();
    void pack();
    bool check();

    void lock();
    void unlock();
    bool try_lock();

    UUID root_shaphot_id() const;
    std::vector<UUID> children_of(const UUID& snapshot_id) const;
    std::vector<std::string> children_of_str(const UUID& snapshot_id) const;
    void remove_named_branch(const std::string& name);

    std::vector<U16String> branch_names();
    UUID branch_head(const U16String& branch_name);

    int32_t snapshot_status(const TxnId& snapshot_id);

    UUID snapshot_parent(const TxnId& snapshot_id);

    U16String snapshot_description(const TxnId& snapshot_id);


    PairPtr& pair();
    const PairPtr& pair() const;
};





template <typename Profile>
class ThreadInMemSnapshot {
    using AllocatorImpl = persistent_inmem::ThreadInMemAllocatorImpl<Profile>;
    using PImpl         = persistent_inmem::ThreadSnapshot<Profile, AllocatorImpl>;
    
    using SnapshotPtr = ThreadInMemSnapshot;
    using TxnId = UUID;

    using AllocatorT = IAllocator<ProfilePageType<Profile>>;
    
    AllocSharedPtr<PImpl> pimpl_;
    
public:
    template <typename CtrName>
    using CtrT = CtrApi<CtrName, Profile>;
    
    using Page = ProfilePageType<Profile>;
    
    
public:
    ThreadInMemSnapshot();
    
    ThreadInMemSnapshot(AllocSharedPtr<PImpl> impl);
    ThreadInMemSnapshot(ThreadInMemSnapshot&& impl);
    
    ThreadInMemSnapshot(const ThreadInMemSnapshot&);
    ThreadInMemSnapshot& operator=(const ThreadInMemSnapshot&);
    
    ~ThreadInMemSnapshot();
    
    ThreadInMemSnapshot& operator=(ThreadInMemSnapshot&&);
    
    bool operator==(const ThreadInMemSnapshot&) const;
    operator bool() const;
    
    ContainerMetadataRepository* metadata() const;
    const UUID& uuid() const;
    bool is_active() const;
    bool is_marked_to_clear() const;
    bool is_committed() const;
    void commit();
    void drop();
    bool drop_ctr(const UUID& name);
    void set_as_master();
    void set_as_branch(U16StringRef name);
    U16String snapshot_metadata() const;
    void set_snapshot_metadata(U16StringRef metadata);
    void lock_data_for_import();
    SnapshotPtr branch();
    bool has_parent() const;
    SnapshotPtr parent();
    void import_new_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    void copy_new_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    void import_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    void copy_ctr_from(ThreadInMemSnapshot<Profile>& txn, const UUID& name);
    bool check();

    Optional<U16String> ctr_type_name_for(const UUID& name);

    void dump_open_containers();
    bool has_open_containers();
    void dump_persistent_tree();

    void walk_containers(ContainerWalker* walker, const char16_t* allocator_descr = nullptr);
    
    UUID clone_ctr(const UUID& name, const UUID& new_name);
    UUID clone_ctr(const UUID& name);

    const PairPtr& pair() const;
    PairPtr& pair();
    
    template <typename CtrName>
    auto find_or_create(const UUID& name)
    {
        return CtrT<CtrName>(snapshot_ref_creation_allowed(), CTR_FIND | CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create(const UUID& name)
    {
        return CtrT<CtrName>(snapshot_ref_creation_allowed(), CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create()
    {
        return CtrT<CtrName>(snapshot_ref_creation_allowed(), CTR_CREATE, CTR_DEFAULT_NAME);
    }

    template <typename CtrName>
    auto find(const UUID& name)
    {
        return CtrT<CtrName>(snapshot_ref_opening_allowed(), CTR_FIND, name);
    }
    
    CtrRef<Profile> get(const UUID& name);
    
    void reset();
    Logger& logger();
    
private:
    AllocSharedPtr<AllocatorT> snapshot_ref_creation_allowed();
    AllocSharedPtr<AllocatorT> snapshot_ref_opening_allowed();
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
