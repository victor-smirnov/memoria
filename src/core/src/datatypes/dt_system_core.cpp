
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

#include <memoria/v1/core/linked/document/ld_common.hpp>

#include <memoria/v1/core/datatypes/default_datatype_ops.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include <memoria/v1/core/datatypes/core.hpp>

#include <string>
#include <cstdlib>
#include <type_traits>
#include <limits>


namespace memoria {
namespace v1 {


#define MMA1_DEFINE_DEFAULT_DATATYPE_OPS(...) \
template <> struct DataTypeOperationsImpl<__VA_ARGS__>: CtrDataTypeOperationsImpl<__VA_ARGS__>{}

MMA1_DEFINE_DEFAULT_DATATYPE_OPS(Decimal);

void InitCoreDatatypes()
{
    register_operations<Decimal>();
}

}}
