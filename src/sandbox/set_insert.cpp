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
#include <memoria/v1/core/tools/ticker.hpp>

#include <algorithm>
#include <vector>
#include <type_traits>
#include <iostream>


using namespace memoria::v1;
using namespace memoria::v1::btss;
using namespace std;





int main()
{
	MEMORIA_INIT(DefaultProfile<>);
	
    using Key   = FixedArray<16>;
    //using Key   = Bytes;


    DInit<Set<Key>>();
	
    try {
        auto alloc = PersistentInMemAllocator<>::create();
		
        auto snp = alloc->master()->branch();

        auto map = create<Set<Key>>(snp);
        
        auto map_name = map->name();
        
        map->setNewPageSize(65536);

        int size = 10000;

        Ticker ticker(100000);

        BigInt t0 = getTimeInMillis();
        BigInt tl = t0;

        for (int c = 0; c < size; c++)
        {
            Key array;//(16);

            for (int c = 0; c < array.length(); c++)
            {
                array[c] = getRandomG(256);
            }

            map->insert_key(array);

            if (ticker.is_threshold())
            {
            	BigInt tt = getTimeInMillis();
            	cout << "Inserted: " << (ticker.ticks() + 1)<< " in " << (tt - tl) << endl;
            	tl = tt;

            	ticker.next();
            }

            ticker.tick();
        }

        BigInt t1 = getTimeInMillis();

        cout << "Inserted " << size << " in " << (t1 - t0) << endl;


        snp->commit();
		snp->set_as_master();


        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("setl_data.dump");
        alloc->store(out.get());
		
        
        auto alloc2 = PersistentInMemAllocator<>::load("setl_data.dump");
        
        auto set1 = find<Set<Key>>(alloc->master(), map_name);

		std::cout << "Ctr size: " << set1->size() << std::endl;
        
        alloc2->dump("alloc2.dump");
    }
    catch (std::exception& ex)
    {
        cout << ex.what() << endl;
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }
	

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();

	/**/
}


