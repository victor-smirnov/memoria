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


#include <memoria/v1/api/allocator/allocator_inmem_threads_api.hpp>
#include <memoria/v1/api/set/set_api.hpp>
#include <memoria/v1/api/vector/vector_api.hpp>

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




int main()
{
    using Key = FixedArray<16>;
    //using Key   = Bytes;

    try {
        auto alloc = ThreadInMemAllocator<>::create();
		
        auto snp = alloc.master().branch();

        auto map = create<Set<Key>>(snp);
        
        std::cout << "Castable to set: " << is_castable_to<Set<Key>>(map.to_ref()) << std::endl;
        std::cout << "Castable to vector: " << is_castable_to<Vector<Key>>(map.to_ref()) << std::endl;
        
        auto map_name = map.name();

        int size = 10000;

        Ticker ticker(100000);

        int64_t t0 = getTimeInMillis();
        int64_t tl = t0;

        for (int c = 0; c < size; c++)
        {
            Key array;//(16);

            for (int c = 0; c < array.length(); c++)
            {
                array[c] = getRandomG(256);
            }

            map.insert(array);

            if (ticker.is_threshold())
            {
            	int64_t tt = getTimeInMillis();
            	cout << "Inserted: " << (ticker.ticks() + 1)<< " in " << (tt - tl) << endl;
            	tl = tt;

            	ticker.next();
            }

            ticker.tick();
        }

        int64_t t1 = getTimeInMillis();

        cout << "Inserted " << size << " in " << (t1 - t0) << endl;
        
        //size_t cnt = 0;
        for (auto ii = map.begin(); !ii.is_end(); ii.next())
        {
            //std::cout << "Row: " << (cnt++) << ": " << ii.key() << std::endl;
        }

        snp.commit();
		snp.set_as_master();

        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("setl_data.dump");
        alloc.store(out.get());
		
        
        auto alloc2 = ThreadInMemAllocator<>::load("setl_data.dump");
        
        auto set_ref = alloc2.master().get(map_name);
        
        auto set1 = cast<Set<Key>>(set_ref);
        
        std::cout << "Set size: " << cast<Set<Key>>(set1.to_ref()).size() << std::endl;

		std::cout << "Ctr size: " << set1.size() << std::endl;
        
        auto iii = set1.begin();
        
        iii.read([](CtrIOBuffer& buffer, int entries){
            for (int c = 0; c < entries; c++) {
                std::cout << IOBufferAdapter<Key>::get(buffer) << std::endl;
            }
            return entries;
        });
        
        alloc2.dump("alloc2.dump");
    }
    catch (std::exception& ex)
    {
        cout << ex.what() << endl;
    }
    catch (MemoriaThrowable& ex)
    {
        ex.dump(std::cout);
    }
}


