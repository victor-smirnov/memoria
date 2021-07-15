
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

#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>
#include <memoria/core/datatypes/type_registry.hpp>
#include <memoria/core/datatypes/datum.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>

#include <memoria/core/tools/fixed_array.hpp>
#include <memoria/core/container/cow.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/api/vector/vector_api.hpp>
#include <memoria/api/map/map_api.hpp>
#include <memoria/api/multimap/multimap_api.hpp>
#include <memoria/api/allocation_map/allocation_map_api.hpp>

#include <memoria/profiles/impl/cow_lite_profile.hpp>
#include <memoria/profiles/impl/cow_profile.hpp>
