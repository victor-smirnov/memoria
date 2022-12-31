
// Copyright 2022 Victor Smirnov
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


#include <memoria/core/strings/format.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/core/tools/random.hpp>

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/hermes/path/path.h>

#include <memoria/memoria.hpp>
#include <memoria/core/tools/time.hpp>

#include <memoria/core/numbers/number_view.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/api/map/map_api.hpp>
#include <memoria/api/vector/vector_api.hpp>

#include <boost/pool/pool_alloc.hpp>
#include <unordered_map>

using namespace memoria;
using namespace memoria;
using namespace memoria::hermes;

int main(int, char**)
{
    InitMemoriaExplicit();

    auto store = create_memory_store();
    auto snp = store->master()->branch();

    auto ctr = create(snp, Vector<Varchar>{});

    ctr->append([](auto& buffer){
        for (size_t c = 0; c < 100; c++) {
            buffer.append("123456789");
        }
        return true;
    });

    ctr->first_entry()->dump(ChunkDumpMode::LEAF);

    return 0;
}
