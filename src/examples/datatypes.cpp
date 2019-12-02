
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


#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/datatypes/datatypes.hpp>

#include <memoria/v1/api/datatypes/varchars/varchar_builder.hpp>
#include <memoria/v1/api/datatypes/buffer/buffer.hpp>

#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/api/vector/vector_api.hpp>

#include <memoria/v1/memoria.hpp>

#include <iostream>

using namespace memoria::v1;

using Ctr1T = Vector<LDDocument>;
using Ctr2T = Vector<Varchar>;


int main()
{
    StaticLibraryCtrs<>::init();

    try {
        auto store = IMemoryStore<>::create();

        auto snp = store->master()->branch();

        auto ctr1 = create(snp, Ctr1T());
        auto ctr2 = create(snp, Ctr2T());

        ctr1->append([&](auto& buffer, size_t size){
            for (size_t c = 0; c < 100; c++)
            {
                buffer.builder().doc().set_sdn("{'booo': 'Hello World', 'foo': [1,2,3,4,5,6,7, " + std::to_string(c) + "]}");
                buffer.builder().build();
            }

            return true;
        });

        ctr2->append([&](auto& buffer, size_t size){
            for (size_t c = 0; c < 100; c++)
            {
                buffer.builder().append("{'booo': 'Hello World', 'foo': [1,2,3,4,5,6,7, " + std::to_string(c) + "]}");
                buffer.builder().build();
            }

            return true;
        });

        std::cout << ctr1->size() << std::endl;
        std::cout << ctr2->size() << std::endl;

        auto scanner1 = ctr1->as_scanner([](auto ctr){
            return ctr->seek(0);
        });

        while (!scanner1.is_end())
        {
            for (const auto& vv: scanner1.values()) {
                std::cout << vv << std::endl;
            }

            scanner1.next_leaf();
        }

        auto scanner2 = ctr2->as_scanner([](auto ctr){
            return ctr->seek(0);
        });

        while (!scanner2.is_end())
        {
            for (const auto& vv: scanner2.values()) {
                std::cout << vv << std::endl;
            }

            scanner2.next_leaf();
        }

        UUID ctr2_id = ctr2->name();

        //ctr2->drop();

        auto ctr3 = find<Ctr1T>(snp, ctr2_id);

        std::cout << ctr3->describe_type() << std::endl;

        snp->commit();
    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cout);
    }

    return 0;
}
