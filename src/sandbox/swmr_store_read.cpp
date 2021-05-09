
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
#include <memoria/memoria.hpp>

#include <iostream>

using namespace memoria;

using CtrType = Set<Varchar>;

int main(void) {
    try {
        const char* file = "file.mma2";

        UUID ctr_id = UUID::parse("e92f1f6d-5ab1-46dc-ba2e-c7718234e71d");

        auto store2 = open_lite_swmr_store(file);

        auto snp2 = store2->begin();
        auto ctr2 = find<CtrType>(snp2, ctr_id);

        ctr2->for_each([](auto view){
            std::cout << view << std::endl;
        });

        snp2->commit();

        StoreCheckCallbackFn callback = [](LDDocument& doc){
            std::cout << doc.to_string() << std::endl;
            return VoidResult::of();
        };

        store2->check(callback);
        store2->close();
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
