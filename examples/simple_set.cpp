
// Copyright 2023 Victor Smirnov
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

// We will be using in-memory store
#include <memoria/api/store/memory_store_api.hpp>
// And a simple Set<> container
#include <memoria/api/set/set_api.hpp>
// Static initialization stuff
#include <memoria/memoria.hpp>

using namespace memoria;

int main(void) {
    // Create a store first. All data is in a store object.
    // We will be using an in-memory store
    auto store = create_memory_store();

    // In-memory store is a confluently-persistent CoW-enabled store,
    // so it suppoerts Git-like branching for containers.
    // We are opening new snapshot from the master branch.
    auto snapshot = store->master()->branch();

    // Now let's create a container, it will be a set of short strings.
    // First, we need to define a datatype for container:
    using DataType = Set<Varchar>;
    // See Hermes docs for more information about datatypes.

    // Now lets create a new set container in our snapshot
    auto set_ctr = create<DataType>(snapshot, DataType{});

    // .. and insert a few strings into it
    for (size_t c = 0; c < 10; c++) {
        set_ctr->upsert(std::string("Entry for ") + std::to_string(c));
    }

    // Now we are ready to iterate over inserted entries.
    set_ctr->for_each([](auto entry){
        println("{}", entry);
    });

    // After we are done with inserting data, we can commit the snapshot
    // so it will be avaliable for other threads to branch from.
    snapshot->commit();

    // And we can store the data into a file
    store->store("set-data.mma");

    // Here all the data will be destroyed in memory,
    // but will remain in the file
}
