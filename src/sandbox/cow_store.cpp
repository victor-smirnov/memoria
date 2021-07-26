
// Copyright 2021 Victor Smirnov
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


#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/filesystem/operations.hpp>
#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;

using CtrType = Set<Varchar>;

int main(void) {
    try {
        const char* file = "file_lmdb.mdb";
        filesystem::remove(file);

        auto store1 = create_memory_store();

        UUID ctr_id = UUID::make_random();
        auto t_start = getTimeInMillis();

        {
            {
                auto snp0 = store1->master()->branch();
                auto ctr0 = create(snp0, CtrType(), ctr_id);

                ctr0->append([&](auto& buf, size_t){
                    for (int c = 0; c < 1000; c++) {
                        buf.append(format_u8("Cool String via Buffer :: {}", c));
                    }
                    return true;
                });

                snp0->commit();
                snp0->set_as_master();

                ctr0->iterator()->dump();
            }

            auto snp2 = store1->master();
            auto ctr2 = find<CtrType>(snp2, ctr_id);

            int cnt = 0;
            int b0  = 0;
            int batch_size = 100;
            int batches = 10000;
            while (cnt < batch_size * batches) {
                auto snp1 = store1->master()->branch();
                auto ctr1 = find<CtrType>(snp1, ctr_id);

                if (b0 % 100 == 0) {
                    std::cout << "Batch " << (b0) << std::endl;
                }

                b0++;

                for (int c = 0; c < batch_size; c++, cnt++) {
                    ctr1->insert((SBuf() << " Cool String ABCDEFGH :: " << cnt).str());
                }

                snp1->commit(ConsistencyPoint::NO);
                snp1->set_as_master();
            }
        }

        auto t_end = getTimeInMillis();

        std::cout << "Store creation time: " << FormatTime(t_end - t_start) << std::endl;

        store1->store("file_cow.mma3");
        store1.reset();

//        auto store2 = open_lmdb_store_readonly(file);

//        auto snp2 = store2->open();
//        auto ctr2 = find<CtrType>(snp2, ctr_id);

//        ctr2->for_each([](auto view){
//            std::cout << view << std::endl;
//        });

////        snp2->commit();
    }
    catch (std::exception& ee) {
        std::cerr << "Exception: " << ee.what() << std::endl;
    }
}
