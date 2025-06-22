
// Copyright 2020-2025 Victor Smirnov
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

#include <memoria/store/oltp/oltp_store_readonly_snapshot_base.hpp>

#include <memoria/api/io/block_level.hpp>

namespace memoria {

template <typename Profile> class OLTPStoreBase;

template <typename Profile>
class BlockIOOLTPStoreReadOnlySnapshotBase: public OLTPStoreReadOnlySnapshotBase<Profile> {
protected:
    using Base = OLTPStoreReadOnlySnapshotBase<Profile>;

    using typename Base::Store;
    using typename Base::CDescrPtr;
    using typename Base::ApiProfileT;
    using typename Base::SharedBlockConstPtr;
    using typename Base::CtrID;
    using typename Base::AllocationMetadataT;

    using Base::snapshot_descriptor_;

    std::shared_ptr<io::BlockIOProvider> blockio_;

public:
    BlockIOOLTPStoreReadOnlySnapshotBase(
        SharedPtr<Store> store,
        std::shared_ptr<io::BlockIOProvider> blockio,
        CDescrPtr& snapshot_descriptor
    ):
        Base(store, snapshot_descriptor),
        blockio_(blockio)
    {}

    ~BlockIOOLTPStoreReadOnlySnapshotBase() noexcept {}
};

}
