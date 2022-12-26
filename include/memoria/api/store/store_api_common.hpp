
// Copyright 2017-2022 Victor Smirnov
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
#include <memoria/core/datatypes/type_signature.hpp>
#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/checks.hpp>
#include <memoria/profiles/common/common.hpp>
#include <memoria/api/common/ctr_api.hpp>


namespace memoria {

template <typename Profile>
class IROStoreSnapshotCtrOps {

public:
    using StoreT = IStoreApiBase<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;
    using CtrReferenceableT = CtrSharedPtr<CtrReferenceable<Profile>>;

    virtual ~IROStoreSnapshotCtrOps() noexcept = default;


    virtual CtrReferenceableT find(const CtrID& ctr_id) = 0;

    virtual bool is_committed() const = 0;
    virtual bool is_active() const = 0;
    virtual bool is_marked_to_clear() const = 0;


    virtual void dump_open_containers() = 0;
    virtual bool has_open_containers() = 0;
    virtual std::vector<CtrID> container_names() const = 0;
    virtual void drop() = 0;
    virtual void check(const CheckResultConsumerFn& consumer) = 0;

    virtual Optional<U8String> ctr_type_name_for(const CtrID& name) = 0;

//    virtual void walk_containers(
//            ContainerWalker<Profile>* walker,
//            const char* allocator_descr = nullptr
//    ) = 0;


protected:

    virtual SnpSharedPtr<StoreT> snapshot_ref_opening_allowed() = 0;
};

enum class ConsistencyPoint {
    AUTO, NO, YES, FULL
};

template <typename Profile>
class IROStoreWritableSnapshotCtrOps: public virtual IROStoreSnapshotCtrOps<Profile> {
    using ApIROStoreBaseT = IStoreApiBase<Profile>;

public:    
    using ROStoreSnapshotPtr = SnpSharedPtr<IROStoreSnapshotCtrOps<Profile>>;
    using CtrID = ApiProfileCtrID<Profile>;
    using CtrReferenceableT = CtrSharedPtr<CtrReferenceable<Profile>>;

    virtual CtrReferenceableT create(const hermes::Datatype& decl, const CtrID& ctr_id) = 0;
    virtual CtrReferenceableT create(const hermes::Datatype& decl) = 0;

    virtual CtrReferenceableT create(U8StringView type_def, const CtrID& ctr_id) {
        auto doc = TypeSignature::parse(type_def);
        auto decl = doc->root().as_datatype();
        return this->create(decl, ctr_id);
    }

    virtual CtrReferenceableT create(U8StringView type_def) {
        auto doc = TypeSignature::parse(type_def);
        auto decl = doc->root().as_datatype();
        return this->create(decl);
    }

    virtual void commit(ConsistencyPoint cp = ConsistencyPoint::AUTO) = 0;

    virtual void flush_open_containers() = 0;

    virtual bool drop_ctr(const CtrID& name) = 0;

    virtual CtrID clone_ctr(const CtrID& name, const CtrID& new_name) = 0;
    virtual CtrID clone_ctr(const CtrID& name) = 0;

    virtual void import_new_ctr_from(ROStoreSnapshotPtr txn, const CtrID& name) = 0;
    virtual void import_ctr_from(ROStoreSnapshotPtr txn, const CtrID& name) = 0;

protected:
    virtual SnpSharedPtr<ApIROStoreBaseT> snapshot_ref_creation_allowed() = 0;
};


template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<IROStoreWritableSnapshotCtrOps<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
)
{
    U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
    auto doc = TypeSignature::parse(signature.to_std_string());
    auto decl = doc->root().as_datatype();

    auto ctr_ref = alloc->create(decl, ctr_id);
    return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref));
}



template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> create(
        SnpSharedPtr<IROStoreWritableSnapshotCtrOps<Profile>> alloc,
        const CtrName& ctr_type_name
)
{
    U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
    auto doc = TypeSignature::parse(signature.to_std_string());
    auto decl = doc->root().as_datatype();
    auto ctr_ref = alloc->create(decl);
    return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref));
}



template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find(
        SnpSharedPtr<IROStoreSnapshotCtrOps<Profile>> alloc,
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
        MEMORIA_MAKE_GENERIC_ERROR(
                    "Container with id {} is not found",
                    ctr_id
        ).do_throw();
    }
}


template <typename CtrName, typename Profile>
CtrSharedPtr<ICtrApi<CtrName, Profile>> find_or_create(
        SnpSharedPtr<IROStoreWritableSnapshotCtrOps<Profile>> alloc,
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

}

