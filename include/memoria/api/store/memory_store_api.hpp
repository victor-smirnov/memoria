
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

#include <memoria/core/types.hpp>
#include <memoria/core/strings/strings.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/container/logs.hpp>
#include <memoria/core/datatypes/type_signature.hpp>
#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/api/store/memory_store_common.hpp>

#include <memoria/api/common/ctr_api.hpp>

#include <boost/filesystem.hpp>

namespace memoria {


template <typename Profile = DefaultProfile<>>
class IMemorySnapshot;

template <typename Profile = DefaultProfile<>>
class IMemoryStore {

    using SnapshotID = ProfileSnapshotID<Profile>;

public:
    using StorePtr      = AllocSharedPtr<IMemoryStore>;
    using SnapshotPtr   = SnpSharedPtr<IMemorySnapshot<Profile>>;
    using BlockType     = ProfileBlockType<Profile>;

    template <typename CtrName>
    using CtrApiType = ICtrApi<CtrName, Profile>;


    virtual ~IMemoryStore() noexcept {}

    static Result<StorePtr> load(InputStreamHandler* input_stream) noexcept;
    static Result<StorePtr> load(U8String file_name) noexcept;

    static Result<StorePtr> create() noexcept;

    virtual int64_t active_snapshots() noexcept = 0;

    virtual Result<void> store(U8String file_name, int64_t wait_duration = 0) noexcept = 0;
    virtual Result<void> store(OutputStreamHandler* output_stream, int64_t wait_duration = 0) noexcept = 0;

    virtual Result<SnapshotPtr> master() noexcept = 0;
    virtual Result<SnapshotPtr> find(const SnapshotID& snapshot_id) noexcept = 0;
    virtual Result<SnapshotPtr> find_branch(U8StringRef name) noexcept = 0;

    virtual Result<void> set_master(const SnapshotID& txn_id) noexcept = 0;
    virtual Result<void> set_branch(U8StringRef name, const SnapshotID& txn_id) noexcept = 0;

    virtual Result<void> walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) noexcept = 0;

    virtual Logger& logger() noexcept = 0;

    virtual Result<void> pack() noexcept = 0;
    virtual Result<bool> check() noexcept = 0;

    virtual void lock() noexcept = 0;
    virtual void unlock() noexcept = 0;
    virtual bool try_lock() noexcept = 0;

    virtual bool is_dump_snapshot_lifecycle() const noexcept = 0;
    virtual void set_dump_snapshot_lifecycle(bool do_dump) noexcept = 0;

    virtual SnapshotID root_shaphot_id() const noexcept = 0;
    virtual Result<std::vector<SnapshotID>> children_of(const SnapshotID& snapshot_id) const noexcept = 0;
    virtual Result<std::vector<std::string>> children_of_str(const SnapshotID& snapshot_id) const noexcept = 0;
    virtual Result<void> remove_named_branch(const std::string& name) noexcept = 0;


    virtual Result<std::vector<SnapshotID>> branch_heads() noexcept = 0;

    virtual Result<std::vector<SnapshotID>> heads() noexcept = 0;
    virtual Result<std::vector<SnapshotID>> heads(const SnapshotID& start_from) noexcept = 0;

    virtual Result<std::vector<SnapshotID>> linear_history(
            const SnapshotID& start_id,
            const SnapshotID& stop_id
    ) noexcept = 0;

    virtual Result<std::vector<U8String>> branch_names() noexcept = 0;
    virtual Result<SnapshotID> branch_head(const U8String& branch_name) noexcept = 0;

    virtual Result<int32_t> snapshot_status(const SnapshotID& snapshot_id) noexcept = 0;

    virtual Result<SnapshotID> snapshot_parent(const SnapshotID& snapshot_id) noexcept = 0;

    virtual Result<U8String> snapshot_description(const SnapshotID& snapshot_id) noexcept = 0;

    virtual PairPtr& pair() noexcept = 0;
    virtual const PairPtr& pair() const noexcept = 0;

    virtual Result<SharedPtr<AllocatorMemoryStat<Profile>>> memory_stat() noexcept = 0;
};

template <typename Profile = DefaultProfile<>>
using IMemoryStorePtr = typename IMemoryStore<Profile>::StorePtr;





template <typename Profile>
class IMemorySnapshot {

    using SnapshotPtr   = SnpSharedPtr<IMemorySnapshot>;
    using SnapshotID    = ProfileSnapshotID<Profile>;
    using CtrID         = ProfileCtrID<Profile>;

    using AllocatorT    = ProfileAllocatorType<Profile>;

public:
    template <typename CtrName>
    using CtrT = ICtrApi<CtrName, Profile>;

    using BlockType = ProfileBlockType<Profile>;

    virtual ~IMemorySnapshot() noexcept {}

    virtual const SnapshotID& uuid() const noexcept = 0;
    virtual bool is_active() const noexcept = 0;
    virtual bool is_marked_to_clear() const noexcept = 0;
    virtual bool is_committed() const noexcept = 0;
    virtual Result<void> commit() noexcept = 0;
    virtual Result<void> drop() noexcept = 0;
    virtual Result<bool> drop_ctr(const CtrID& name) noexcept = 0;
    virtual Result<void> set_as_master() noexcept = 0;
    virtual Result<void> set_as_branch(U8StringRef name) noexcept = 0;
    virtual U8String snapshot_metadata() const noexcept = 0;
    virtual Result<void> set_snapshot_metadata(U8StringRef metadata) noexcept = 0;
    virtual Result<void> lock_data_for_import() noexcept = 0;
    virtual Result<SnapshotPtr> branch() noexcept = 0;
    virtual bool has_parent() const noexcept = 0;
    virtual Result<SnapshotPtr> parent() noexcept = 0;
    virtual Result<void> import_new_ctr_from(SnapshotPtr txn, const CtrID& name) noexcept = 0;
    virtual Result<void> copy_new_ctr_from(SnapshotPtr txn, const CtrID& name) noexcept = 0;
    virtual Result<void> import_ctr_from(SnapshotPtr txn, const CtrID& name) noexcept = 0;
    virtual Result<void> copy_ctr_from(SnapshotPtr txn, const CtrID& name) noexcept = 0;
    virtual Result<bool> check() noexcept = 0;

    virtual Result<void> flush_open_containers() noexcept = 0;

    virtual Result<Optional<U8String>> ctr_type_name_for(const CtrID& name) noexcept = 0;

    virtual Result<std::vector<CtrID>> container_names() const noexcept = 0;
    virtual Result<std::vector<U8String>> container_names_str() const noexcept = 0;

    virtual Result<void> dump_dictionary_blocks() noexcept = 0;
    virtual Result<void> dump_open_containers() noexcept = 0;
    virtual Result<bool> has_open_containers() noexcept = 0;
    virtual Result<void> dump_persistent_tree() noexcept = 0;

    virtual Result<void> walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) noexcept = 0;

    virtual Result<CtrID> clone_ctr(const CtrID& name, const CtrID& new_name) noexcept = 0;
    virtual Result<CtrID> clone_ctr(const CtrID& name) noexcept = 0;

    virtual const PairPtr& pair() const noexcept = 0;
    virtual PairPtr& pair() noexcept = 0;

    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> create(const LDTypeDeclarationView& decl, const CtrID& ctr_id) noexcept = 0;
    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> create(const LDTypeDeclarationView& decl) noexcept = 0;
    virtual Result<CtrSharedPtr<CtrReferenceable<Profile>>> find(const CtrID& ctr_id) noexcept = 0;

    virtual Logger& logger() noexcept = 0;
    virtual Result<SharedPtr<SnapshotMemoryStat<Profile>>> memory_stat() noexcept = 0;

protected:
    virtual Result<SnpSharedPtr<AllocatorT>> snapshot_ref_creation_allowed() noexcept = 0;
    virtual Result<SnpSharedPtr<AllocatorT>> snapshot_ref_opening_allowed() noexcept = 0;
};


template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> create(
        SnpSharedPtr<IMemorySnapshot<Profile>>& alloc,
        const CtrName& ctr_type_name,
        const ProfileCtrID<Profile>& ctr_id
) noexcept
{
    using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
    return wrap_throwing([&] {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        LDDocument doc = TypeSignature::parse(signature.to_std_string());
        LDTypeDeclarationView decl = doc.value().as_type_decl();
        Result<CtrSharedPtr<CtrReferenceable<Profile>>> ctr_ref = alloc->create(decl, ctr_id);

        if (ctr_ref.is_ok())
        {
            return ResultT::of(memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(ctr_ref.get()));
        }
        else {
            return std::move(ctr_ref).transfer_error();
        }
    });
}

template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> create(
        SnpSharedPtr<IMemorySnapshot<Profile>>& alloc,
        const CtrName& ctr_type_name
) noexcept
{
    using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
    return wrap_throwing([&] () -> ResultT {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        LDDocument doc = TypeSignature::parse(signature.to_std_string());
        LDTypeDeclarationView decl = doc.value().as_type_decl();
        Result<CtrSharedPtr<CtrReferenceable<Profile>>> ctr_ref = alloc->create(decl);

        if (ctr_ref.is_ok())
        {
            return ResultT::of(memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(ctr_ref.get()));
        }
        else {
            return std::move(ctr_ref).transfer_error();
        }
    });
}

template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> find(
        SnpSharedPtr<IMemorySnapshot<Profile>>& alloc,
        const ProfileCtrID<Profile>& ctr_id
) noexcept
{
    using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
    return wrap_throwing([&] {

        Result<CtrSharedPtr<CtrReferenceable<Profile>>> ctr_ref = alloc->find(ctr_id);
        MEMORIA_RETURN_IF_ERROR(ctr_ref);

        U8String signature = make_datatype_signature<CtrName>().name();

        if (ctr_ref.get()->describe_datatype() == signature) {
            return ResultT::of(memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(ctr_ref.get()));
        }
        else {
            return ResultT::make_error(
                "Container type mismatch. Expected: {}, actual: {}",
                signature,
                ctr_ref->get().describe_datatype()
            );
        }
    });
}


}

