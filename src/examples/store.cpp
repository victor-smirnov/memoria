
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


#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/memoria.hpp>

#include <iostream>

using namespace memoria::v1;

int main()
{
    // Initializing Containers' Metadata (for statically linked libMemoriaClassic)
    // Dynamic linking does not require this step.
    StaticLibraryCtrs<>::init();

    // Creating new instance of in-memory store.
    auto alloc = IMemoryStore<>::create().get_or_terminate();

    // Obtaining Master's branch head.
    auto master = alloc->master().get_or_terminate();

    // Creating new writable snapshot from the Master's head.
    auto snp = master->branch().get_or_terminate();

    // Do somethig with containers here

    // Committing snapshot. From this moment, all data is immutable.
    snp->commit().throw_if_error();

    // Chaning Master's head to point to the new snapshot
    snp->set_as_master().throw_if_error();

    // Saving store's data to the file.
    alloc->store("store.mma2").throw_if_error();

    return 0;
}
