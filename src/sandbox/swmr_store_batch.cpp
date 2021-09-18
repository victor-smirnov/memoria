
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
#include <memoria/filesystem/operations.hpp>
#include <memoria/core/datatypes/buffer/buffer.hpp>


#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;

using CtrType = Set<Varchar>;

int main(void) {
    try {
        const char* file = "file.mma2";

        filesystem::remove(file);
        auto store1 = create_swmr_store(file, 1024);

        UID256 ctr_id = UID256::parse("e92f1f6d-5ab1-46dc-ba2e-c7718234e71d");

        auto t_start = getTimeInMillis();

        CheckResultConsumerFn callback = [](CheckSeverity, const LDDocument& doc){
            std::cout << doc.to_string() << std::endl;
            return VoidResult::of();
        };

        {
            auto snp1 = store1->begin();
            auto ctr1 = create<CtrType>(snp1, CtrType(), ctr_id);

            ctr1->append([&](auto& buf, size_t) -> bool {
                for (int c = 0; c < 1000; c++) {
                    buf.append(format_u8("Cool string {}", c));
                }

                return true;
            });


            ctr1->for_each([](auto key){
                println("{}", U8String(key));
            });

            snp1->commit(ConsistencyPoint::NO);
        }

        store1->check(callback);

        auto t_end = getTimeInMillis();

        std::cout << "Store creation time: " << FormatTime(t_end - t_start) << std::endl;

        store1->check(callback);

        auto vv = create_graphviz_dot_visitor("swmr-write.dot");
        store1->traverse(*vv.get());

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
