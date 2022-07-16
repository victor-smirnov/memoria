
// Copyright 2021-2022 Victor Smirnov
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

#include "store_tools.hpp"

#include <iostream>

using namespace memoria;

//using StorePtrT = AllocSharedPtr<ILMDBStore<CoreApiProfile>>;
using StorePtrT = AllocSharedPtr<ISWMRStore<CoreApiProfile>>;
//using StorePtrT = AllocSharedPtr<IMemoryStore<CoreApiProfile>>;

int main(void) {


    Seed(123456);
    SeedBI(123456);

    try {
        U8String file = "file.mma2";

//        auto store = std::make_shared<LMDBStoreOperation>(file, 10240);
        auto store = std::make_shared<LiteSWMRStoreOperation>(file, 1024);
//        auto store = std::make_shared<MemoryStoreOperation>(file);


        store->set_remove_existing_file(false);

        StoreTestBench<StorePtrT> bench(store);


        bench.set_entries(10000);
        bench.set_batch_size(100);
        bench.set_check_epocs(true);
        bench.set_consistency_point(ConsistencyPoint::AUTO);

        bench.run_insertions();

        bench.run_queries();
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
