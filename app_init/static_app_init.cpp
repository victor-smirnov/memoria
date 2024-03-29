
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



#include <memoria/memoria_app_init.hpp>
#include <memoria/memoria_core.hpp>
#include <memoria/memoria_ctrs.hpp>
#include <memoria/memoria_stores.hpp>

namespace memoria {

MMA_DEFINE_EXPLICIT_CU_LINKING(MemoriaStaticAppInit)

struct StaticInitializer {
    StaticInitializer() {
        InitMemoriaCoreExplicit();
        InitProfileMetadata();
        init_core_containers();
        InitMemoriaStoresExplicit();
    }
};

namespace {
StaticInitializer init0;
}

void InitMemoriaExplicit() {
    StaticInitializer init0;
}

}
