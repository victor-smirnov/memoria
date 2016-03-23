// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/memoria.hpp>

#include <memoria/v1/containers/map/map_factory.hpp>



#include <memory>

using namespace memoria;
using namespace std;




int main() {
    MEMORIA_INIT(DefaultProfile<>);

    using Key   = BigInt;
    using Value = BigInt;

    DInit<Map<Key, Value>>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp = alloc->master()->branch();

        try {

            auto map = create<Map<Key, Value>>(snp);

            int from = -10000;
            int to   =  10000;

            for (int c = from; c < to; c++) {
                // map->assign(toString(c), c);
                //map->assign(c, toString(c));
                map->assign(c, c);
            }

            FSDumpAllocator(snp, "mapx_orig.dir");

            MEMORIA_ASSERT(map->size(), ==, to - from);

            check_snapshot(snp);

            for (auto c = map->begin(); !c->is_end(); c->next())
            {
                cout << c->key() << " -- " << c->value() << endl;
            }

            for (int c = from; c < to; c++)
            {
                map->remove(c);
            }

            cout << "After remove" << endl;

            for (auto c = map->begin(); !c->is_end(); c->next())
            {
                cout << c->key() << " -- " << c->value() << endl;
            }

            MEMORIA_ASSERT(map->size(), ==, 0);

            snp->commit();

            check_snapshot(snp);

            FSDumpAllocator(snp, "mapx.dir");
        }
        catch (...) {
            FSDumpAllocator(snp, "mapx.dir");
            throw;
        }
    }
    catch (v1::Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
