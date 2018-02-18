
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

#include <memoria/v1/filesystem/path.hpp>

#include "allocator_inmem_api_common.hpp"

namespace memoria {
namespace v1 {
    

namespace persistent_inmem {
    template <typename Profile> class InMemAllocatorImpl;
    template <typename Profile, typename PersistentAllocator> class Snapshot;
}

template <typename Profile = DefaultProfile<>>
class InMemSnapshot;
	
template <typename Profile = DefaultProfile<>>
class InMemAllocator {
    using PImpl = persistent_inmem::InMemAllocatorImpl<Profile>;
    using TxnId = UUID;
    
    AllocSharedPtr<PImpl> pimpl_;
public:
    using SnapshotPtr   = InMemSnapshot<Profile>;
    using Page          = ProfilePageType<Profile>;
    
    InMemAllocator();
    
    InMemAllocator(AllocSharedPtr<PImpl> impl);
    InMemAllocator(InMemAllocator&& impl);
    
    InMemAllocator(const InMemAllocator&);
    InMemAllocator& operator=(const InMemAllocator&);
    
    ~InMemAllocator();
    
    InMemAllocator& operator=(InMemAllocator&&);
    
    bool operator==(const InMemAllocator&) const;
    operator bool() const;
    
    
    static InMemAllocator load(InputStreamHandler* input_stream);
    static InMemAllocator load(InputStreamHandler* input_stream, int32_t cpu);
    
    static InMemAllocator load(memoria::v1::filesystem::path file_name);
    static InMemAllocator load(memoria::v1::filesystem::path file_name, int32_t cpu);
    
    static InMemAllocator create();
    static InMemAllocator create(int32_t cpu);
    
    void store(filesystem::path file_name);
    void store(OutputStreamHandler* output_stream);
    
    SnapshotPtr master();
    SnapshotPtr find(const TxnId& snapshot_id);
    SnapshotPtr find_branch(U16StringRef name);
    
    void set_master(const TxnId& txn_id);
    void set_branch(U16StringRef name, const TxnId& txn_id);
    
    ContainerMetadataRepository* metadata() const;
    void walk_containers(ContainerWalker* walker, const char16_t* allocator_descr = nullptr);
    
    void dump(filesystem::path dump_at);
    
    Logger& logger();
    
    void reset();
    void pack();
    bool check();
    
    SnpSharedPtr<SnapshotMetadata<TxnId>> describe(const TxnId& snapshot_id) const;
    SnpSharedPtr<SnapshotMetadata<TxnId>> describe_master() const;
};





template <typename Profile>
class InMemSnapshot {
    using AllocatorImpl = persistent_inmem::InMemAllocatorImpl<Profile>;
    using PImpl         = persistent_inmem::Snapshot<Profile, AllocatorImpl>;
    
    using SnapshotPtr = InMemSnapshot;
    using TxnId = UUID;

    using AllocatorT = IAllocator<ProfilePageType<Profile>>;
    
    AllocSharedPtr<PImpl> pimpl_;
    
public:
    template <typename CtrName>
    using CtrT = CtrApi<CtrName, Profile>;
    
    using Page = ProfilePageType<Profile>;
    
    
public:
    InMemSnapshot();
    
    InMemSnapshot(AllocSharedPtr<PImpl> impl);
    InMemSnapshot(InMemSnapshot&& impl);
    
    InMemSnapshot(const InMemSnapshot&);
    InMemSnapshot& operator=(const InMemSnapshot&);
    
    ~InMemSnapshot();
    
    InMemSnapshot& operator=(InMemSnapshot&&);
    
    bool operator==(const InMemSnapshot&) const;
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
    void import_new_ctr_from(InMemSnapshot<Profile>& txn, const UUID& name);
    void copy_new_ctr_from(InMemSnapshot<Profile>& txn, const UUID& name);
    void import_ctr_from(InMemSnapshot<Profile>& txn, const UUID& name);
    void copy_ctr_from(InMemSnapshot<Profile>& txn, const UUID& name);
    bool check();
    void dump(filesystem::path destination);
    void dump_persistent_tree();
    void walk_containers(ContainerWalker* walker, const char16_t* allocator_descr = nullptr);
    
    
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
    
    
    SnpSharedPtr<SnapshotMetadata<UUID>> describe() const;
    
private:
    AllocSharedPtr<AllocatorT> snapshot_ref_creation_allowed();
    AllocSharedPtr<AllocatorT> snapshot_ref_opening_allowed();
};




template <typename CtrName, typename Profile>
auto create(InMemSnapshot<Profile>& alloc, const UUID& name)
{
    return alloc.template create<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto create(InMemSnapshot<Profile>&& alloc, const UUID& name)
{
    return alloc.template create<CtrName>(name);
}



template <typename CtrName, typename Profile>
auto create(InMemSnapshot<Profile>& alloc)
{
    return alloc.template create<CtrName>();
}

template <typename CtrName, typename Profile>
auto create(InMemSnapshot<Profile>&& alloc)
{
    return alloc.template create<CtrName>();
}



template <typename CtrName, typename Profile>
auto find_or_create(InMemSnapshot<Profile>& alloc, const UUID& name)
{
    return alloc.template find_or_create<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto find_or_create(InMemSnapshot<Profile>&& alloc, const UUID& name)
{
    return alloc.template find_or_create<CtrName>(name);
}



template <typename CtrName, typename Profile>
auto find(InMemSnapshot<Profile>& alloc, const UUID& name)
{
    return alloc.template find<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto find(InMemSnapshot<Profile>&& alloc, const UUID& name)
{
    return alloc.template find<CtrName>(name);
}



}
}
