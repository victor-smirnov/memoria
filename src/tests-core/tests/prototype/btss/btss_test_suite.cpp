
// Copyright 2015 Victor Smirnov
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

#include <memoria/profiles/core_cow_api/core_cow_api_profile.hpp>

#include "btss_batch_test.hpp"

#include <memoria/api/vector/vector_api.hpp>
#include <memoria/api/set/set_api.hpp>

namespace memoria {
namespace tests {

auto Suite1 = register_class_suite<BTSSBatchTest<Vector<UTinyInt>>>("BTSS.Vector.UTinyInt");
auto Suite2 = register_class_suite<BTSSBatchTest<Vector<Varchar>>>("BTSS.Vector.Varchar");
auto Suite3 = register_class_suite<BTSSBatchTest<Set<Varchar>>>("BTSS.Set.Varchar");

}}
