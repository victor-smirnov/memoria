
// Copyright 2021 Victor Smirnov
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

#include <memoria/api/map/map_api.hpp>
#include <memoria/api/store/swmr_store_api.hpp>
#include <memoria/profiles/core_api/core_api_profile.hpp>

#include <memoria/store/swmr/common/swmr_store_datatypes.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>

#include <memoria/profiles/impl/cow_lite_profile.hpp>
#include <memoria/profiles/impl/cow_profile.hpp>

#include <stack>
#include <fstream>

namespace memoria {

using ApiProfileT = CoreApiProfile<>;

using CommitMetadata_DT = CommitMetadataDT<DataTypeFromProfile<ApiProfileT>>;

using HistoryCtrType = Map<UUID, CommitMetadata_DT>;

MMA_DEFINE_DEFAULT_DATATYPE_OPS(CommitMetadata_DT);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(HistoryCtrType);

void InitCoreApiSWMRDatatypes() {
    register_operations<CommitMetadata_DT>();

    register_notctr_operations<HistoryCtrType>();


    InitCtrMetadata<HistoryCtrType, CowProfile<>>();
    InitCtrMetadata<HistoryCtrType, CowLiteProfile<>>();
}


}
