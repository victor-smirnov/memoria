
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
#include <memoria/v1/core/container/logs.hpp>
#include <memoria/v1/api/datatypes/type_signature.hpp>
#include <memoria/v1/api/datatypes/traits.hpp>


#include <memoria/v1/profiles/common/common.hpp>

#include <memoria/v1/api/allocator/allocator_inmem_api_common.hpp>

#include <memoria/v1/api/common/ctr_api.hpp>

#include <boost/filesystem.hpp>

namespace memoria {
namespace v1 {



template <typename Profile = DefaultProfile<>>
class IMemorySnapshot;

template <typename Profile = DefaultProfile<>>
class IMemoryStore {

    using SnapshotID = ProfileSnapshotID<Profile>;
    using StorePtr   = AllocSharedPtr<IMemoryStore>;

public:
    using SnapshotPtr   = SnpSharedPtr<IMemorySnapshot<Profile>>;
    using BlockType     = ProfileBlockType<Profile>;


    virtual ~IMemoryStore() noexcept {}

    static StorePtr load(InputStreamHandler* input_stream);
    static StorePtr load(U8String file_name);

    //static StorePtr load(boost::filesystem::path file_name);

    static StorePtr create();

    virtual int64_t active_snapshots() = 0;

    //virtual void store(boost::filesystem::path file_name, int64_t wait_duration = 0) = 0;

    virtual void store(U8String file_name, int64_t wait_duration = 0) = 0;
    virtual void store(OutputStreamHandler* output_stream, int64_t wait_duration = 0) = 0;

    virtual SnapshotPtr master() = 0;
    virtual SnapshotPtr find(const SnapshotID& snapshot_id) = 0;
    virtual SnapshotPtr find_branch(U16StringRef name) = 0;

    virtual void set_master(const SnapshotID& txn_id) = 0;
    virtual void set_branch(U16StringRef name, const SnapshotID& txn_id) = 0;

    virtual void walk_containers(ContainerWalker<Profile>* walker, const char16_t* allocator_descr = nullptr) = 0;

    virtual Logger& logger() = 0;



    virtual void pack() = 0;
    virtual bool check() = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual bool try_lock() = 0;

    virtual bool is_dump_snapshot_lifecycle() const = 0;
    virtual void set_dump_snapshot_lifecycle(bool do_dump) = 0;

    virtual SnapshotID root_shaphot_id() const = 0;
    virtual std::vector<SnapshotID> children_of(const SnapshotID& snapshot_id) const = 0;
    virtual std::vector<std::string> children_of_str(const SnapshotID& snapshot_id) const = 0;
    virtual void remove_named_branch(const std::string& name) = 0;


    virtual std::vector<SnapshotID> heads() = 0;

    virtual std::vector<U16String> branch_names() = 0;
    virtual SnapshotID branch_head(const U16String& branch_name) = 0;

    virtual int32_t snapshot_status(const SnapshotID& snapshot_id) = 0;

    virtual SnapshotID snapshot_parent(const SnapshotID& snapshot_id) = 0;

    virtual U16String snapshot_description(const SnapshotID& snapshot_id) = 0;

    virtual PairPtr& pair() = 0;
    virtual const PairPtr& pair() const = 0;

    virtual SharedPtr<AllocatorMemoryStat> memory_stat(bool include_containers = true) = 0;
};



template <typename Profile>
class IMemorySnapshot {

    using SnapshotPtr   = SnpSharedPtr<IMemorySnapshot>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileSnapshotID<Profile>;

    using AllocatorT    = ProfileAllocatorType<Profile>;

public:
    template <typename CtrName>
    using CtrT = CtrApi<CtrName, Profile>;

    using BlockType = ProfileBlockType<Profile>;

    virtual ~IMemorySnapshot() noexcept {}

    virtual const SnapshotID& uuid() const = 0;
    virtual bool is_active() const = 0;
    virtual bool is_marked_to_clear() const = 0;
    virtual bool is_committed() const = 0;
    virtual void commit() = 0;
    virtual void drop() = 0;
    virtual bool drop_ctr(const CtrID& name) = 0;
    virtual void set_as_master() = 0;
    virtual void set_as_branch(U16StringRef name) = 0;
    virtual U16String snapshot_metadata() const = 0;
    virtual void set_snapshot_metadata(U16StringRef metadata) = 0;
    virtual void lock_data_for_import() = 0;
    virtual SnapshotPtr branch() = 0;
    virtual bool has_parent() const = 0;
    virtual SnapshotPtr parent() = 0;
    virtual void import_new_ctr_from(SnapshotPtr txn, const CtrID& name) = 0;
    virtual void copy_new_ctr_from(SnapshotPtr txn, const CtrID& name) = 0;
    virtual void import_ctr_from(SnapshotPtr txn, const CtrID& name) = 0;
    virtual void copy_ctr_from(SnapshotPtr txn, const CtrID& name) = 0;
    virtual bool check() = 0;

    virtual Optional<U16String> ctr_type_name_for(const CtrID& name) = 0;

    virtual std::vector<CtrID> container_names() const = 0;
    virtual std::vector<U16String> container_names_str() const = 0;

    virtual void dump_dictionary_blocks() = 0;
    virtual void dump_open_containers() = 0;
    virtual bool has_open_containers() = 0;
    virtual void dump_persistent_tree() = 0;

    virtual void walk_containers(ContainerWalker<Profile>* walker, const char16_t* allocator_descr = nullptr) = 0;

    virtual CtrID clone_ctr(const CtrID& name, const CtrID& new_name) = 0;
    virtual CtrID clone_ctr(const CtrID& name) = 0;

    virtual const PairPtr& pair() const = 0;
    virtual PairPtr& pair() = 0;

    virtual CtrSharedPtr<CtrReferenceable> create_ctr(const DataTypeDeclaration& decl, const CtrID& ctr_id) = 0;
    virtual CtrSharedPtr<CtrReferenceable> create_ctr(const DataTypeDeclaration& decl) = 0;

    virtual CtrSharedPtr<CtrReferenceable> find_ctr(const DataTypeDeclaration& decl, const CtrID& ctr_id) = 0;
    virtual CtrSharedPtr<CtrReferenceable> find_or_create_ctr(const DataTypeDeclaration& decl, const CtrID& ctr_id) = 0;

    template <typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, Profile>> create_ctr(const CtrName& ctr_type_name, const CtrID& ctr_id)
    {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        DataTypeDeclaration decl = TypeSignature::parse(signature.to_std_string());
        CtrSharedPtr<CtrReferenceable> ctr_ref = create_ctr(decl, ctr_id);
        return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(ctr_ref);
    }

    template <typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, Profile>> create_ctr(const CtrName& ctr_type_name)
    {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        DataTypeDeclaration decl = TypeSignature::parse(signature.to_std_string());
        CtrSharedPtr<CtrReferenceable> ctr_ref = create_ctr(decl);
        return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(ctr_ref);
    }

    template <typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, Profile>> find_ctr(const CtrName& ctr_type_name, const CtrID& ctr_id)
    {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        DataTypeDeclaration decl = TypeSignature::parse(signature.to_std_string());
        CtrSharedPtr<CtrReferenceable> ctr_ref = find_ctr(decl, ctr_id);
        return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(ctr_ref);
    }

    template <typename CtrName>
    CtrSharedPtr<ICtrApi<CtrName, Profile>> find_or_create_ctr(const CtrName& ctr_type_name, const CtrID& ctr_id)
    {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        DataTypeDeclaration decl = TypeSignature::parse(signature.to_std_string());
        CtrSharedPtr<CtrReferenceable> ctr_ref = find_or_create_ctr(decl, ctr_id);
        return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(ctr_ref);
    }


    template <typename CtrName>
    auto find_or_create(const CtrID& name)
    {
        return CtrT<CtrName>(snapshot_ref_creation_allowed(), CTR_FIND | CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create(const CtrID& name)
    {
        return CtrT<CtrName>(snapshot_ref_creation_allowed(), CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create()
    {
        return CtrT<CtrName>(snapshot_ref_creation_allowed(), CTR_CREATE, CTR_DEFAULT_NAME);
    }

    template <typename CtrName>
    auto find(const CtrID& name)
    {
        return CtrT<CtrName>(snapshot_ref_opening_allowed(), CTR_FIND, name);
    }

    //virtual CtrRef<Profile> get_ctr(const CtrID& name) = 0;


    virtual Logger& logger() = 0;

    virtual SharedPtr<SnapshotMemoryStat> memory_stat(bool include_containers = true) = 0;

protected:
    virtual SnpSharedPtr<AllocatorT> snapshot_ref_creation_allowed() = 0;
    virtual SnpSharedPtr<AllocatorT> snapshot_ref_opening_allowed() = 0;
};



template <typename CtrName, typename Profile>
auto create(SnpSharedPtr<IMemorySnapshot<Profile>>& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template create<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto create(SnpSharedPtr<IMemorySnapshot<Profile>>&& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template create<CtrName>(name);
}



template <typename CtrName, typename Profile>
auto create(SnpSharedPtr<IMemorySnapshot<Profile>>& alloc)
{
    return alloc->template create<CtrName>();
}

template <typename CtrName, typename Profile>
auto create(SnpSharedPtr<IMemorySnapshot<Profile>>&& alloc)
{
    return alloc->template create<CtrName>();
}



template <typename CtrName, typename Profile>
auto find_or_create(SnpSharedPtr<IMemorySnapshot<Profile>>& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template find_or_create<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto find_or_create(SnpSharedPtr<IMemorySnapshot<Profile>>&& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template find_or_create<CtrName>(name);
}



template <typename CtrName, typename Profile>
auto find(SnpSharedPtr<IMemorySnapshot<Profile>>& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template find<CtrName>(name);
}

template <typename CtrName, typename Profile>
auto find(SnpSharedPtr<IMemorySnapshot<Profile>>&& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template find<CtrName>(name);
}




}
}
