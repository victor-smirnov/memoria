// Copyright 2011-2021 Victor Smirnov
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

namespace memoria {


#ifdef MEMORIA_BUILD_MEMORY_STORE
void InitDefaultInMemStore();
#endif

#if defined(MEMORIA_BUILD_MEMORY_STORE_COW)
void InitCoWInMemStore();
#endif

#if defined(MEMORIA_BUILD_SWMR_STORE_MAPPED)
void InitSWMRMappedStore();
#endif

void InitMemoriaStoresExplicit();

}
