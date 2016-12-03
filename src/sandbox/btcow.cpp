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

#include <memoria/v1/containers/map/map_factory.hpp>

#include <memoria/v1/prototypes/bt_cow/btcow_factory.hpp>

#include <memory>

using namespace memoria::v1;
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

//            auto map = create<Map<Key, Value>>(snp);
//
//            int from = -10000;
//            int to   =  10000;
//
//            for (int c = from; c < to; c++) {
//                // map->assign(toString(c), c);
//                //map->assign(c, toString(c));
//                map->assign(c, c);
//            }
//
//            FSDumpAllocator(snp, "mapx_orig.dir");
//
//            MEMORIA_V1_ASSERT(map->size(), ==, to - from);
//
//            check_snapshot(snp);
//
//            for (auto c = map->begin(); !c->is_end(); c->next())
//            {
//                cout << c->key() << " -- " << c->value() << endl;
//            }
//
//            for (int c = from; c < to; c++)
//            {
//                map->remove(c);
//            }
//
//            cout << "After remove" << endl;
//
//            for (auto c = map->begin(); !c->is_end(); c->next())
//            {
//                cout << c->key() << " -- " << c->value() << endl;
//            }
//
//            MEMORIA_V1_ASSERT(map->size(), ==, 0);

            snp->commit();

            check_snapshot(snp);

            FSDumpAllocator(snp, "btcow.dir");
        }
        catch (...) {
            FSDumpAllocator(snp, "btcow.dir");
            throw;
        }
    }
    catch (Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
