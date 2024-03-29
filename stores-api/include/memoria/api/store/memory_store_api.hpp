
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

#include <memoria/core/strings/strings.hpp>
#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/checks.hpp>

#include <memoria/api/store/memory_store_common.hpp>

#include <memoria/api/common/ctr_api.hpp>
#include <memoria/api/store/store_api_common.hpp>

#include <boost/filesystem.hpp>

namespace memoria {


template <typename Profile>
class IMemorySnapshot;

template <typename Profile>
class IMemoryStore {

    using SnapshotID = ApiProfileSnapshotID<Profile>;

public:
    using StorePtr      = AllocSharedPtr<IMemoryStore>;
    using SnapshotPtr   = SnpSharedPtr<IMemorySnapshot<Profile>>;
    using ReadOnlySnapshotPtr = SnapshotPtr;
    using WritableSnapshotPtr = SnapshotPtr;
    using ProfileT = Profile;

    template <typename CtrName>
    using CtrApiType = ICtrApi<CtrName, Profile>;

    virtual ~IMemoryStore() noexcept {}

    virtual int64_t active_snapshots() = 0;

    virtual void store(U8String file_name, int64_t wait_duration = 0) = 0;
    virtual void store(OutputStreamHandler* output_stream, int64_t wait_duration = 0) = 0;

    virtual SnapshotPtr master() = 0;
    virtual SnapshotPtr find(const SnapshotID& snapshot_id) = 0;
    virtual SnapshotPtr find_branch(U8StringRef name) = 0;

    virtual void set_master(const SnapshotID& txn_id) = 0;
    virtual void set_branch(U8StringRef name, const SnapshotID& txn_id) = 0;

    //virtual void walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) = 0;

    virtual void pack() = 0;
    virtual void check(const CheckResultConsumerFn& consumer) = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual bool try_lock() = 0;

    virtual bool is_dump_snapshot_lifecycle() const = 0;
    virtual void set_dump_snapshot_lifecycle(bool do_dump) = 0;

    virtual SnapshotID root_shaphot_id() const = 0;
    virtual std::vector<SnapshotID> children_of(const SnapshotID& snapshot_id) const = 0;
    virtual std::vector<std::string> children_of_str(const SnapshotID& snapshot_id) const = 0;
    virtual void remove_named_branch(const std::string& name) = 0;


    virtual std::vector<SnapshotID> branch_heads() = 0;

    virtual std::vector<SnapshotID> heads() = 0;
    virtual std::vector<SnapshotID> heads(const SnapshotID& start_from) = 0;

    virtual std::vector<SnapshotID> linear_history(
            const SnapshotID& start_id,
            const SnapshotID& stop_id
    ) = 0;

    virtual std::vector<U8String> branch_names() = 0;
    virtual SnapshotID branch_head(const U8String& branch_name) = 0;

    virtual int32_t snapshot_status(const SnapshotID& snapshot_id) = 0;

    virtual SnapshotID snapshot_parent(const SnapshotID& snapshot_id) = 0;

    virtual U8String snapshot_description(const SnapshotID& snapshot_id) = 0;

    virtual PairPtr& pair() = 0;
    virtual const PairPtr& pair() const = 0;

    virtual SharedPtr<StoreMemoryStat<Profile>> memory_stat() = 0;
};

template <typename Profile = CoreApiProfile>
using IMemoryStorePtr = typename IMemoryStore<Profile>::StorePtr;




template <typename Profile>
class IMemorySnapshot: public IROStoreWritableSnapshotCtrOps<Profile> {

    using Base = IROStoreWritableSnapshotCtrOps<Profile>;

    using SnapshotPtr   = SnpSharedPtr<IMemorySnapshot>;
    using SnapshotID    = ApiProfileSnapshotID<Profile>;
    using CtrID         = ApiProfileCtrID<Profile>;

    using StoreT    = IStoreApiBase<Profile>;

public:
    using typename Base::ROStoreSnapshotPtr;

    template <typename CtrName>
    using CtrT = ICtrApi<CtrName, Profile>;

    virtual const SnapshotID& uuid() const = 0;

    virtual void set_as_master() = 0;
    virtual void set_as_branch(U8StringRef name) = 0;
    virtual U8String snapshot_metadata() const = 0;
    virtual void set_snapshot_metadata(U8StringRef metadata) = 0;
    virtual void lock_data_for_import() = 0;
    virtual SnapshotPtr branch() = 0;
    virtual bool has_parent() const = 0;
    virtual SnapshotPtr parent() = 0;
    //virtual void import_new_ctr_from(SnapshotPtr txn, const CtrID& name) = 0;
    virtual void copy_new_ctr_from(ROStoreSnapshotPtr txn, const CtrID& name) = 0;
    //virtual void import_ctr_from(SnapshotPtr txn, const CtrID& name) = 0;
    virtual void copy_ctr_from(ROStoreSnapshotPtr txn, const CtrID& name) = 0;

    virtual std::vector<U8String> container_names_str() const = 0;

    virtual void dump_dictionary_blocks() = 0;
    virtual void dump_persistent_tree() = 0;

    virtual const PairPtr& pair() const = 0;
    virtual PairPtr& pair() = 0;

    virtual SharedPtr<SnapshotMemoryStat<Profile>> memory_stat() = 0;
};


template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<IMemorySnapshot<Profile>>& alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
)
{
    U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
    auto doc = TypeSignature::parse(signature.to_std_string());
    auto decl = doc.root().value().as_datatype();

    auto ctr_ref = alloc->create(decl, ctr_id);
    (void)ctr_ref;
    return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref));
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<IMemorySnapshot<Profile>>& alloc,
        const CtrName& ctr_type_name
)
{
    U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
    auto doc = TypeSignature::parse(signature.to_std_string());
    auto decl = doc.root().value().as_datatype();
    auto ctr_ref = alloc->create(decl);
    (void)ctr_ref;
    return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref));
}

template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<IMemorySnapshot<Profile>>& alloc,
        const ApiProfileCtrID<Profile>& ctr_id
)
{
    auto ctr_ref = alloc->find(ctr_id);

    if (ctr_ref)
    {
        U8String signature = make_datatype_signature<CtrName>().name();

        if (ctr_ref->describe_datatype() == signature) {
            return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref));
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                        "Container type mismatch. Expected: {}, actual: {}",
                        signature,
                        ctr_ref->describe_datatype()
                        ).do_throw();
        }
    }
    else {
        return CtrSharedPtr<ICtrApi<CtrName, Profile>>{};
    }
}


template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find_or_create(
        SnpSharedPtr<IMemorySnapshot<Profile>>& alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
)
{
    auto type_name = alloc->ctr_type_name_for(ctr_id);
    if (type_name) {
        return find<CtrName>(alloc, ctr_id);
    }
    else {
        return create<CtrName>(alloc, ctr_type_name, ctr_id);
    }
}

SharedPtr<IMemoryStore<CoreApiProfile>> create_memory_store();
SharedPtr<IMemoryStore<CoreApiProfile>> load_memory_store(U8String path);
SharedPtr<IMemoryStore<CoreApiProfile>> load_memory_store(InputStreamHandler* input_stream);
bool is_memory_store(U8String path);

SharedPtr<IMemoryStore<CoreApiProfile>> create_memory_store_cowlite();
SharedPtr<IMemoryStore<CoreApiProfile>> load_memory_store_cowlite(U8String path);
SharedPtr<IMemoryStore<CoreApiProfile>> load_memory_store_cowlite(InputStreamHandler* input_stream);
bool is_memory_store_cowlite(U8String path);

}

