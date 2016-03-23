// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Idiomatic Map<K, V> example for Memoria. Map is an ordered mapping between keys and values
// backed with b-tree like structure. Besides traditional key-based lookup, Map<> also supports
// entry lookup by entry index from the beginning and bulk insertion/removal.

// Not every object type is supported as a key or value. In short, custom value codec or field factory
// must be defined. Out of the box, all primitive types are supported, String, BigInteger and some
// others (not yet implemented ones). Fixed-size types are somewhat faster on updates than
// variable-length (String, BigInteger) ones.


#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>

using namespace memoria;
using namespace std;

int main()
{
    // Initialize common metadata. Every Memoria program must use this macro.
    MEMORIA_INIT(DefaultProfile<>);

    using Key   = BigInt;
    using Value = String;

    // Every Memoria container has some static metadata used for I/O. Before a container is instantiated
    // this metadata must be initialized.
    DInit<Map<Key, Value>>();

    try {
        // Create persistent in-memory allocator for containers to store their data in.
        auto alloc = PersistentInMemAllocator<>::create();

        // This callocator is persistent not because it can dump its content to stream or file, but in a
        // narrow sense: to make a group of changes to allocator one must first start new snapshot by
        // branching form any exiting one's.

        // Snapshots are completely isolated from each other, operations don't interfere with operations
        // on other snapshots.

        auto snp = alloc->master()->branch();


        // Create Map
        auto map = create<Map<Key, Value>>(snp);

        int from = -300;
        int to   =  300;

        // Fill map element by element.
        for (int c = from; c < to; c++)
        {
            map->assign(c, "str_" + toString(c));
        }

        // Iterator on the whole container
        for (auto iter = map->begin(); !iter->is_end(); iter->next())
        {
            cout << iter->key() << " -- " << iter->value() << endl;
        }


        // Dump readable contents of allocator to disk to see what is under the hood.
        // Mostly usable for hacking and debugging.
        FSDumpAllocator(snp, "map_full.dir");

        // Remove all entries by keys
        for (int c = from; c < to; c++)
        {
            map->remove(c);
        }

        FSDumpAllocator(snp, "map_empty.dir");

        // Finish snapshot so no other updates are possible.
        snp->commit();

        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("map_data.dump");
        alloc->store(out.get());
    }
    catch (memoria::Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();
}
