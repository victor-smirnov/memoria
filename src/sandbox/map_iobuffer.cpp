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


#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

using namespace memoria::v1;
using namespace std;

class PrintingConsumer: public bt::BufferConsumer<IOBuffer> {

	IOBuffer buffer_;

public:
	PrintingConsumer(size_t size): buffer_(size) {}

	virtual IOBuffer& buffer() {
		return buffer_;
	}

	virtual Int process(IOBuffer& buffer, Int entries)
	{
		for (Int c = 0; c < entries; c++)
		{
			cout << "Key: '" << buffer.getString() << "' Value: '" << buffer.getString() << "'" << endl;
		}

		return entries;
	}
};




int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using Key   = String;
    using Value = String;

    DInit<Map<Key, Value>>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();

        auto snp = alloc->master()->branch();



        auto map = create<Map<Key, Value>>(snp);

        int from =  0;
        int to   =  300000;

        // Fill map element by element.
        for (int c = from; c < to; c++)
        {
            map->assign("key_" + toString(c), "value_" + toString(c));
        }

//        FSDumpAllocator(snp, "map_full.dir");


        PrintingConsumer consumer(65536);

        auto iter = map->begin();

        iter->read_buffer(&consumer);

        // Finish snapshot so no other updates are possible.
        snp->commit();

        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("map_data.dump");
        alloc->store(out.get());
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();
}
