
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

#include <memoria/api/store/swmr_store_api.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/filesystem/operations.hpp>
#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;

using CtrType = Set<Varchar>;

int main(void) {
    try {
        const char* file = "file.mma2";

        filesystem::remove(file);
        auto store1 = create_lite_swmr_store(file, 256);

        UID256 ctr_id = UID256::make_random();

        auto t_start = getTimeInMillis();

        CheckResultConsumerFn callback = [](CheckSeverity svr, const LDDocument& doc){
            std::cout << doc.to_string() << std::endl;
            if (svr == CheckSeverity::ERROR) {
                MEMORIA_MAKE_GENERIC_ERROR("Container check error").do_throw();
            }
        };

        {
            {
                auto snp0 = store1->begin();
                auto ctr0 = create(snp0, CtrType(), ctr_id);
                ctr0->set_ctr_property("CtrName", "SimpleCtr");
                snp0->set_transient(true);
                snp0->commit();
            }

            int cnt = 0;
            int b0  = 0;
            int batch_size = 1000;
            int num_entries = 100000;

            while (cnt < num_entries)
            {
                if (b0 % (num_entries / batch_size / 100) == 0)
                {
                    std::cout << "Batch " << (b0) << " :: " << cnt << " :: " << (num_entries) << std::endl;
                    store1->check(callback);
                }

                auto snp1 = store1->begin();
                auto ctr1 = find<CtrType>(snp1, ctr_id);

                for (int c = 0; c < batch_size; c++, cnt++)
                {
                    auto val = getBIRandomG();

                    U8String str = (SBuf() << "Cool String ABCDEFGH :: " << val).str();

                    ctr1->insert(str);

                    if (!ctr1->contains(str)) {
                        MEMORIA_MAKE_GENERIC_ERROR("Can't find key {} in the container", str).do_throw();
                    }
                }

                //snp1->set_transient(false);
                snp1->commit(ConsistencyPoint::AUTO);

                b0++;
            }
        }

        store1->flush();

        auto t_end = getTimeInMillis();
        std::cout << "Store creation time: " << FormatTime(t_end - t_start) << std::endl;

        store1->check(callback);

        store1->close();
    }
    catch (const MemoriaError& ee) {
        ee.describe(std::cout);
    }
    catch (const MemoriaThrowable& ee) {
        ee.dump(std::cout);
    }
    catch (const std::exception& ee) {
        std::cerr << "Exception: " << ee.what() << std::endl;
    }
}
