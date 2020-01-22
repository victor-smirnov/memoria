
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


#include <memoria/profiles/default/default.hpp>
#include <memoria/core/datatypes/datatypes.hpp>

#include <memoria/core/datatypes/varchars/varchar_builder.hpp>
#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/api/vector/vector_api.hpp>

#include <memoria/memoria.hpp>

#include <memoria/core/tools/result.hpp>

#include <iostream>

using namespace memoria;

using Ctr1T = Vector<LinkedData>;
using Ctr2T = Vector<Varchar>;

template <typename... Args>
Result<void> boo(Args...) {
    return Result<void>::simple_memoria_error("Ooops!");
}



int main()
{
    try {
        Result<int> rr = wrap_throwing([] () -> Result<int> {
            //auto boo_r = boo();
            //MEMORIA_RETURN_IF_ERROR(boo(1,2,3,4));
            //return 12345;

            //return Result<EmptyType>::of();
            MMA_THROW(RuntimeException()) << WhatCInfo("Foooo!");
        });

        rr.get_or_terminate();

        //std::cout << rr << std::endl;

        //rr.throw_if_error();

        //std::cout << rr.throw_if_error() << std::endl;



//        LDDocument doc = LDDocument::parse("Decimal(10,8)");
//        doc.dump(std::cout) << std::endl;

        //std::cout << cast_as<Varchar>(doc.value()) << std::endl;

        //DataTypeRegistryStore::global().register_creator_fn<Decimal, TL<int32_t, int32_t>, TL<int32_t>, TL<>>();

        //DataTypeRegistry::local().create_object(doc.value().as_type_decl());

        /*
        auto store = IMemoryStore<>::create();

        auto snp = store->master()->branch();

        auto ctr1 = create(snp, Ctr1T());
        auto ctr2 = create(snp, Ctr2T());

        ctr1->append([&](auto& buffer, size_t size){
            for (size_t c = 0; c < 1; c++)
            {
                buffer.builder().doc().set_sdn("{'booo': 'Hello World', 'foo': [ [], {}, 1,2,3,4,5,6,7, " + std::to_string(c) + "], 'key0': false}");
                buffer.builder().build();
            }

            return true;
        });

        ctr2->append([&](auto& buffer, size_t size){
            for (size_t c = 0; c < 1; c++)
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

        //auto ctr3 = find<Ctr1T>(snp, ctr2_id);
        //std::cout << ctr3->describe_type() << std::endl;

        snp->commit();

        /**/
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

    return 0;
}
