
// Copyright 2018 Victor Smirnov
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




#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/fixed_array.hpp>
#include <memoria/api/set/set_api.hpp>
#include <memoria/api/map/map_api.hpp>

#include <memoria/api/allocator/allocator_inmem_api.hpp>

#include <memoria/reactor/application.hpp>


using namespace memoria::v1;
using namespace memoria::reactor;


int main(int argc, char** argv, char** envp)
{
    Application app(argc, argv, envp);
    app.start_engines();

    return app.run([&]{

        ShutdownOnScopeExit exh;

        InMemAllocator<> alloc = InMemAllocator<>::create();

        auto bb = alloc.master().branch();

        auto fset = create<Set<FixedArray<16>>>(bb);
        auto smap = create<Map<U8String, U8String>>(bb);

        FixedArray<16> array;

        for (int c = 0; c < 10000; c++)
        {
            for (int d = 0; d < 16; d++)
            {
                array[d] = getRandomG(255);
            }

            fset.insert(array);
            smap.assign(toString(c) + "_key", toString(c) + "_value");
        }

        bb.set_snapshot_metadata("My cool snapshot");

        bb.commit();
        bb.set_as_master();

        alloc.store("sample-alloc.mma1");

        return 0;
    });
}
