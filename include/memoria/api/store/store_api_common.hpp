
// Copyright 2017-2021 Victor Smirnov
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
#include <memoria/profiles/common/common.hpp>
#include <memoria/api/common/ctr_api.hpp>


namespace memoria {

template <typename Profile>
class IStoreSnapshotCtrOps {

public:
    using AllocatorT = AllocatorApiBase<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrSharedPtr<CtrReferenceable<Profile>>>;

    virtual ~IStoreSnapshotCtrOps() noexcept = default;


    virtual CtrReferenceableResult find(const CtrID& ctr_id) noexcept = 0;

    virtual bool is_committed() const noexcept = 0;
    virtual bool is_active() const noexcept = 0;
    virtual bool is_marked_to_clear() const noexcept = 0;


    virtual VoidResult dump_open_containers() noexcept = 0;
    virtual BoolResult has_open_containers() noexcept = 0;
    virtual Result<std::vector<CtrID>> container_names() const noexcept = 0;
    virtual VoidResult drop() noexcept = 0;
    virtual BoolResult check() noexcept = 0;

    virtual Result<Optional<U8String>> ctr_type_name_for(const CtrID& name) noexcept = 0;

//    virtual VoidResult walk_containers(
//            ContainerWalker<Profile>* walker,
//            const char* allocator_descr = nullptr
//    ) noexcept = 0;


protected:

    virtual Result<SnpSharedPtr<AllocatorT>> snapshot_ref_opening_allowed() noexcept = 0;
};


template <typename Profile>
class IStoreWritableSnapshotCtrOps: public virtual IStoreSnapshotCtrOps<Profile> {
    using AllocatorT = AllocatorApiBase<Profile>;

public:
    using CtrID = ApiProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrSharedPtr<CtrReferenceable<Profile>>>;

    virtual CtrReferenceableResult create(const LDTypeDeclarationView& decl, const CtrID& ctr_id) noexcept = 0;
    virtual CtrReferenceableResult create(const LDTypeDeclarationView& decl) noexcept = 0;

    virtual CtrReferenceableResult create(U8StringView type_def, const CtrID& ctr_id) noexcept {
        return wrap_throwing([&] () -> CtrReferenceableResult {
            LDDocument doc = TypeSignature::parse(type_def);
            LDTypeDeclarationView decl = doc.value().as_type_decl();
            return this->create(decl, ctr_id);
        });
    }

    virtual CtrReferenceableResult create(U8StringView type_def) noexcept {
        return wrap_throwing([&] () -> CtrReferenceableResult {
            LDDocument doc = TypeSignature::parse(type_def);
            LDTypeDeclarationView decl = doc.value().as_type_decl();
            return this->create(decl);
        });
    }

    virtual VoidResult commit(bool flush = true) noexcept = 0;
    virtual VoidResult flush_open_containers() noexcept = 0;

    virtual BoolResult drop_ctr(const CtrID& name) noexcept = 0;

    virtual Result<CtrID> clone_ctr(const CtrID& name, const CtrID& new_name) noexcept = 0;
    virtual Result<CtrID> clone_ctr(const CtrID& name) noexcept = 0;

protected:
    virtual Result<SnpSharedPtr<AllocatorT>> snapshot_ref_creation_allowed() noexcept = 0;
};


template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> create(
        SnpSharedPtr<IStoreWritableSnapshotCtrOps<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
) noexcept
{
    using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
    return wrap_throwing([&] () -> ResultT {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        LDDocument doc = TypeSignature::parse(signature.to_std_string());
        LDTypeDeclarationView decl = doc.value().as_type_decl();

        MEMORIA_TRY(ctr_ref, alloc->create(decl, ctr_id));
        (void)ctr_ref;
        return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref_result));
    });
}



template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> create(
        SnpSharedPtr<IStoreWritableSnapshotCtrOps<Profile>> alloc,
        const CtrName& ctr_type_name
) noexcept
{
    using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
    return wrap_throwing([&] () -> ResultT {
        U8String signature = make_datatype_signature<CtrName>(ctr_type_name).name();
        LDDocument doc = TypeSignature::parse(signature.to_std_string());
        LDTypeDeclarationView decl = doc.value().as_type_decl();
        MEMORIA_TRY(ctr_ref, alloc->create(decl));
        (void)ctr_ref;
        return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref_result));
    });
}



template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> find(
        SnpSharedPtr<IStoreSnapshotCtrOps<Profile>> alloc,
        const ApiProfileCtrID<Profile>& ctr_id
) noexcept
{
    using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
    return wrap_throwing([&] () -> ResultT {

        MEMORIA_TRY(ctr_ref, alloc->find(ctr_id));

        U8String signature = make_datatype_signature<CtrName>().name();

        if (ctr_ref->describe_datatype() == signature) {
            return memoria_static_pointer_cast<ICtrApi<CtrName, Profile>>(std::move(ctr_ref_result));
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "Container type mismatch. Expected: {}, actual: {}",
                signature,
                ctr_ref->describe_datatype()
            );
        }
    });
}


template <typename CtrName, typename Profile>
Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>> find_or_create(
        SnpSharedPtr<IStoreWritableSnapshotCtrOps<Profile>> alloc,
        const CtrName& ctr_type_name,
        const ApiProfileCtrID<Profile>& ctr_id
) noexcept
{
    using ResultT = Result<CtrSharedPtr<ICtrApi<CtrName, Profile>>>;
    return wrap_throwing([&] () -> ResultT {
        MEMORIA_TRY(type_name, alloc->ctr_type_name_for(ctr_id));
        if (type_name) {
            return find<CtrName>(alloc, ctr_id);
        }
        else {
            return create<CtrName>(alloc, ctr_type_name, ctr_id);
        }
    });
}



}

