// Copyright 2016 Victor Smirnov
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




#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/set/set_factory.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include <algorithm>
#include <vector>
#include <type_traits>


using namespace memoria::v1;
using namespace memoria::v1::btss;
using namespace std;



int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using Key   = FixedArray<16>;

    DInit<Set<Key>>();


    try {
        auto alloc = PersistentInMemAllocator<>::create();

        alloc->lock();
        alloc->unlock();
        alloc->try_lock();

        auto snp1 = alloc->master()->branch();
        auto snp2 = alloc->master()->branch();

        auto set1 = create<Set<Key>>(snp1);
        auto set2 = create<Set<Key>>(snp2);

        snp1->commit();

        snp2->move_ctr_from(snp1, set1->name());

        snp2->commit();
        snp2->set_as_master();

        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("setf_data.dump");
        alloc->store(out.get());
        FSDumpAllocator(alloc, "snapshots.dir");
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();
}
