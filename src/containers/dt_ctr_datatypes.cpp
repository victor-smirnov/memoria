
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>
#include <memoria/core/datatypes/type_registry.hpp>
#include <memoria/core/datatypes/datum.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>

#include <memoria/core/tools/fixed_array.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/api/vector/vector_api.hpp>
#include <memoria/api/map/map_api.hpp>
#include <memoria/api/multimap/multimap_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/profiles/impl/cow_lite_profile.hpp>

#include <string>

namespace memoria {

MMA_DEFINE_DEFAULT_DATATYPE_OPS(Set<FixedArray<16>>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Set<Varchar>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Set<UUID>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Vector<Varchar>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Vector<UTinyInt>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Map<Varchar, Varchar>);

MMA_DEFINE_DEFAULT_DATATYPE_OPS(Vector<LinkedData>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Map<BigInt, Varchar>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Map<BigInt, BigInt>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Map<UUID, UUID>);
//MMA_DEFINE_DEFAULT_DATATYPE_OPS(Multimap<BigInt, UTinyInt>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Multimap<UUID, UTinyInt>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(Multimap<Varchar, Varchar>);
MMA_DEFINE_DEFAULT_DATATYPE_OPS(AllocationMap);

template <typename Profile>
using DirectoryCtrType = Map<ProfileCtrID<Profile>, ProfileBlockID<Profile>>;
MMA_DEFINE_DEFAULT_DATATYPE_OPS(DirectoryCtrType<CowLiteProfile<>>);

void InitCtrDatatypes()
{
    register_notctr_operations<Set<FixedArray<16>>>();

    register_notctr_operations<Set<Varchar>>();
    register_notctr_operations<Set<UUID>>();

    register_notctr_operations<Vector<Varchar>>();
    register_notctr_operations<Vector<UTinyInt>>();
    register_notctr_operations<Map<Varchar, Varchar>>();

    register_notctr_operations<Vector<LinkedData>>();
    register_notctr_operations<Map<BigInt, Varchar>>();
    register_notctr_operations<Map<BigInt, BigInt>>();
    register_notctr_operations<Map<UUID, UUID>>();
    //register_notctr_operations<Multimap<BigInt, UTinyInt>>();
    register_notctr_operations<Multimap<UUID, UTinyInt>>();
    register_notctr_operations<Multimap<Varchar, Varchar>>();
    register_notctr_operations<AllocationMap>();
    register_notctr_operations<DirectoryCtrType<CowLiteProfile<>>>();
}

}
