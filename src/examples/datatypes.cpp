
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

#include <memoria/api/store/swmr_store_api.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/memoria.hpp>


#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace memoria;

using Profile = MemoryCoWProfile<>;


int main()
{
    using CtrName = Set<Varchar>;

    long dtrStart{};

    try {
        UUID uuid = UUID::make_random();

        std::cout << uuid.hi() << " :: " << uuid.lo() << std::endl;
        std::cout << uuid << std::endl;

        const char* name = "/home/victor/memoria_swmr_file.mma4";
        memoria::filesystem::remove(name);
        auto store1 = create_mapped_swmr_store(name, 1024).get_or_throw();

        UUID ctr_id = UUID::make_random();

        auto txn = store1->begin().get_or_throw();
        auto ctr = create(txn->ops(), CtrName{}, ctr_id).get_or_throw();

        for (int c = 0; c < 10000; c++) {
            ctr->insert(format_u8("AAAAAAAAAAAAAAAAAA: {}", c)).get_or_throw();
        }

        txn->commit().get_or_throw();

        store1.reset();

        auto store2 = open_mapped_swmr_store(name).get_or_throw();

        auto txn2 = store2->open().get_or_throw();
        auto ctr2 = find<CtrName>(txn2->ro_ops(), ctr_id).get_or_throw();

        std::cout << ctr2->size().get_or_throw() << std::endl;
        ctr2->for_each([](auto key){
            std::cout << "Key: " << key << std::endl;
        }).get_or_throw();


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
