
// Copyright 2019 Victor Smirnov
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


#include <memoria/core/datatypes/datatypes.hpp>
#include <memoria/profiles/memory_cow/memory_cow_profile.hpp>
#include <memoria/api/store/memory_store_api.hpp>
#include <memoria/api/set/set_api.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/memoria.hpp>

#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace memoria;

using Profile = MemoryCoWProfile<>;


int main()
{
    using CtrName = Set<Varchar>;

    InitCtrMetadata<CtrName, MemoryCoWProfile<>>();

    long dtrStart;

    try {
        auto store = IMemoryStore<Profile>::create().get_or_throw();

        auto master = store->master().get_or_throw();
        auto snp = master->branch().get_or_throw();

        UUID name = UUID::make_random();

        auto ctr = create(snp, CtrName{}, name).get_or_throw();

        std::vector<U8String> entries;

        for (int c = 0; c < 10000000; c++) //47793
        {
            U8String entry = format_u8("BBBBBBBBB_{}", c);
            entries.push_back(entry);
        }

        std::random_device rd;
        std::mt19937 g(rd());

        std::shuffle(entries.begin(), entries.end(), g);

        long t0 = getTimeInMillis();

        int cnt = 0;
        for (auto& entry: entries) {
            if (cnt % 10000 == 0) {
                std::cout << "C = " << cnt << std::endl;
            }

            ctr->insert(entry).get_or_throw();
            cnt++;
        }

        long t1 = getTimeInMillis();

        std::cout << "Insertion time: " << (t1 - t0) << std::endl;

        snp->commit().get_or_throw();
        snp->set_as_master().get_or_throw();

        store->store("set_cow.mma3").get_or_throw();

        auto store2 = IMemoryStore<Profile>::load("set_cow.mma3").get_or_throw();

        auto snp2 = store2->master().get_or_throw()->branch().get_or_throw();

        auto ctr1 = find<CtrName>(snp2, name).get_or_throw();

        long t2 = getTimeInMillis();


        for (auto& entry: entries) {
            if (!ctr1->contains(entry).get_or_throw()) {
                std::cout << "Missing " << entry << std::endl;
            }
        }

        long t3 = getTimeInMillis();

        std::cout << "Query time: " << (t3 - t2) << std::endl;



//        cnt = 0;
//        for (auto& entry: entries)
//        {
//            if (cnt % 10000 == 0)
//            {
//                std::cout << "R = " << cnt << std::endl;
//            }

//            ctr1->remove(entry).get_or_throw();
//            cnt++;
//        }

//        std::cout << "Final size: " << ctr1->size().get_or_throw() << std::endl;
//        std::cout << "Prev size: " << ctr->size().get_or_throw() << std::endl;

//        for (auto& entry: entries) {
//            if (!ctr->contains(entry).get_or_throw()) {
//                std::cout << "Missing " << entry << std::endl;
//            }
//        }

        //snp2->drop().get_or_throw();

        dtrStart = getTimeInMillis();
    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cout);
    }
    catch (std::exception& ex) {
        std::cout << "std::exception has been thrown: " << ex.what() << std::endl;
    }
    catch (int& vv) {
        std::cout << "Int has been thrown: " << vv << std::endl;
    }

    long dtrEnd = getTimeInMillis();

    std::cout << "Dtr time: " << (dtrEnd - dtrStart) << std::endl;

    return 0;
}
