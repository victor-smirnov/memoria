
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


#include <memoria/profiles/memory_cow/memory_cow_profile.hpp>
#include <memoria/api/store/swmr_store_api.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/filesystem/operations.hpp>
#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;

using CtrType = Set<Varchar>;

int main(void) {
    try {
        const char* file = "file.mma2";

        filesystem::remove(file);
        auto store1 = create_mapped_swmr_store(file, 1024).get_or_throw();

        UUID ctr_id = UUID::make_random();

        auto t_start = getTimeInMillis();

        StoreCheckCallbackFn callback = [](LDDocument& doc){
            std::cout << doc.to_string() << std::endl;
            return VoidResult::of();
        };

        {
            {
                auto snp0 = store1->begin().get_or_throw();
                auto ctr0 = create(snp0, CtrType(), ctr_id).get_or_throw();
                snp0->commit().get_or_throw();
                //snp0->describe_to_cout().get_or_throw();
            }


            int cnt = 0;
            int b0  = 0;
            int batch_size = 100;
            int batches = 10;
            while (cnt < batch_size * batches) {
                auto snp1 = store1->begin().get_or_throw();
                auto ctr1 = find<CtrType>(snp1, ctr_id).get_or_throw();

                if (b0 % 100 == 0) {
                    std::cout << "Batch " << (b0) << std::endl;
                }

                b0++;

                for (int c = 0; c < batch_size; c++, cnt++) {
                    ctr1->insert((SBuf() << " Cool String ABCDEFGH :: " << cnt).str()).get_or_throw();
                }

                snp1->commit(false).get_or_throw();
            }
        }

        store1->check(callback).get_or_throw();

        store1->close().get_or_throw();

        auto t_end = getTimeInMillis();

        std::cout << "Store creation time: " << FormatTime(t_end - t_start) << std::endl;

//        auto store2 = open_mapped_swmr_store(file).get_or_throw();

//        auto snp2 = store2->begin().get_or_throw();
//        auto ctr2 = find<CtrType>(snp2, ctr_id).get_or_throw();

//        ctr2->for_each([](auto view){
//            std::cout << view << std::endl;
//        }).get_or_throw();

//        snp2->commit().get_or_throw();

//        store2->check(callback).get_or_throw();

//        store2->close().get_or_throw();
    }
    catch (std::exception& ee) {
        std::cerr << "Exception: " << ee.what() << std::endl;
    }
}
