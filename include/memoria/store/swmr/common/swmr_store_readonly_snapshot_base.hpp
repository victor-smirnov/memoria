
// Copyright 2020-2021 Victor Smirnov
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
#include <memoria/store/swmr/common/swmr_store_snapshot_base.hpp>

namespace memoria {

template <typename Profile> class SWMRStoreBase;

template <typename Profile>
class SWMRStoreReadOnlySnapshotBase: public SWMRStoreSnapshotBase<Profile> {
protected:
    using Base = SWMRStoreSnapshotBase<Profile>;

    using typename Base::Store;
    using typename Base::CDescrPtr;
    using typename Base::ApiProfileT;
    using typename Base::SharedBlockConstPtr;
    using typename Base::CtrID;
    using typename Base::AllocationMetadataT;

    using Base::snapshot_descriptor_;

public:
    SWMRStoreReadOnlySnapshotBase(
            SharedPtr<Store> store,
            CDescrPtr& snapshot_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate
    )  :
        Base(store, snapshot_descriptor, refcounter_delegate)
    {}


    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const ld::LDTypeDeclarationView& decl,
            const CtrID& ctr_id
    ){
        return this->create_ctr_instance(decl, ctr_id);
    }

    bool is_transient()  {
        return snapshot_descriptor_->is_transient();
    }

    bool is_system_snapshot()  {
        return snapshot_descriptor_->is_system_snapshot();
    }

    void start_no_reentry(const CtrID& ctr_id) {}
    void finish_no_reentry(const CtrID& ctr_id) noexcept {}

    void register_allocation(const AllocationMetadataT& alc) {
        this->store_->register_allocation(alc);
    }
};

}
