
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

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/datatypes/default_datatype_ops.hpp>
#include <memoria/v1/core/datatypes/type_registry.hpp>
#include <memoria/v1/core/datatypes/datum.hpp>

#include <memoria/v1/core/datatypes/default_datatype_ops.hpp>

#include <memoria/v1/core/tools/fixed_array.hpp>

#include <memoria/v1/api/set/set_api.hpp>
#include <memoria/v1/api/vector/vector_api.hpp>
#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/api/multimap/multimap_api.hpp>

#include <string>

namespace memoria {
namespace v1 {

#define MMA1_DEFINE_DEFAULT_DATATYPE_OPS(...) \
template <> struct DataTypeOperationsImpl<__VA_ARGS__>: CtrDataTypeOperationsImpl<__VA_ARGS__>{}

MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Set<FixedArray<16>>);
MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Set<Varchar>);
MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Vector<Varchar>);
MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Vector<UTinyInt>);
MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Map<Varchar, Varchar>);

MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Vector<LinkedData>);
MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Map<BigInt, Varchar>);
MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Map<BigInt, BigInt>);
//MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Multimap<BigInt, UTinyInt>);
//MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Multimap<UUID, UTinyInt>);
MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Multimap<Varchar, Varchar>);

void InitCtrDatatypes()
{
    register_notctr_operations<Set<FixedArray<16>>>();

    register_notctr_operations<Set<Varchar>>();
    register_notctr_operations<Vector<Varchar>>();
    register_notctr_operations<Vector<UTinyInt>>();
    register_notctr_operations<Map<Varchar, Varchar>>();

    register_notctr_operations<Vector<LinkedData>>();
    register_notctr_operations<Map<BigInt, Varchar>>();
    register_notctr_operations<Map<BigInt, BigInt>>();
    //register_notctr_operations<Multimap<BigInt, UTinyInt>>();
    //register_notctr_operations<Multimap<UUID, UTinyInt>>();
    register_notctr_operations<Multimap<Varchar, Varchar>>();
}

}}
