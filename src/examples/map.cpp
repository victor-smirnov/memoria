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


// Idiomatic Map<K, V> example for Memoria. Map is an ordered mapping between keys and values
// backed with b-tree like structure. Besides traditional key-based lookup, Map<> also supports
// entry lookup by entry index from the beginning and bulk insertion/removal.

// Not every object type is supported as a key or value. In short, custom value codec or field factory
// must be defined. Out of the box, all primitive types are supported, String, BigInteger and some
// others (not yet implemented ones). Fixed-size types are somewhat faster on updates than
// variable-length (String, BigInteger) ones.


#include <memoria/v1/api/allocator/allocator_inmem_api.hpp>
#include <memoria/v1/api/map/map_api.hpp>

#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/reactor/file_streams.hpp>

using namespace memoria::v1;
using namespace memoria::v1::reactor;
using namespace std;

int main(int argc, char** argv)
{
    using Key   = int64_t;
    using Value = U8String;

    return Application::run(argc, argv, []
    {
        try {            
            // Create persistent in-memory allocator for containers to store their data in.
            auto alloc = InMemAllocator<>::create();

            // This callocator is persistent not because it can dump its content to stream or file, but in a
            // narrow sense: to make a group of changes to allocator one must first start new snapshot by
            // branching form any exiting one's.

            // Snapshots are completely isolated from each other, operations don't interfere with operations
            // on other snapshots.

            auto snp = alloc.master().branch();


            // Create Map
            auto map = create<Map<Key, Value>>(snp);

            int from = -300;
            int to   =  300;

            // Fill map element by element.
            for (int c = from; c < to; c++)
            {
                map.assign(c, U8String("str_") + toString(c));
            }

//             // Iterator on the whole container
//             for (auto iter = map.begin(); !iter.is_end(); iter.next())
//             {
//                 cout << iter.key() << " -- " << iter.value() << endl;
//             }

//             int cnt = 0;
//             auto entries = map.begin().read(map.size());
//             
//             for (auto& entry: entries)
//             {
//                 std::cout << (cnt++) << " -- " << std::get<0>(entry) << ": " << std::get<1>(entry) << std::endl;
//             }
            

//             // Dump readable contents of allocator to disk to see what is under the hood.
//             // Mostly usable for hacking and debugging.
//             snp.dump("map_full.dir");
// 
//             // Remove all entries by keys
//             for (int c = from; c < to; c++)
//             {
//                 map.remove(c);
//             }
// 
            snp.dump("map_empty.dir");

            // Finish snapshot so no other updates are possible.
            snp.commit();

            // Store binary contents of allocator to the file.
            auto out = FileOutputStreamHandler::create_buffered("map_data.dump");
            alloc.store(out.get());
        }
        catch (MemoriaThrowable& ex)
        {
            ex.dump(std::cout);
        }
        catch (std::exception& ex)
        {
            cout << ex.what() << endl;
        }
    
        app().shutdown();
        
        return 0;
    });
}
