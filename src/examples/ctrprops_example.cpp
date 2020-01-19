
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


#include <memoria/v1/core/datatypes/datatypes.hpp>

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/memoria.hpp>

#include <iostream>

using namespace memoria::v1;

int main()
{
    StaticLibraryCtrs<>::init();

    try {

        using MapType = Map<Varchar, Varchar>;
        //using Entry   = std::pair<U8String, U8String>;

        auto alloc = IMemoryStore<>::create().get_or_throw();

        auto snp = alloc->master().get_or_throw()->branch().get_or_throw();

        auto ctr0 = create(snp, MapType()).get_or_throw();

        std::string value1(16, 'X');
        std::string value2(17, 'Y');
        std::string value3(18, 'Z');

        ctr0->set_ctr_property("prop1", value1).throw_if_error();
        ctr0->set_ctr_property("prop2", value2).throw_if_error();
        ctr0->set_ctr_property("prop3", value3).throw_if_error();

        ctr0->for_each_ctr_property([](auto key_view, auto value_view){
            std::cout << "Prop: " << key_view << " :: " << value_view << std::endl;
        }).throw_if_error();

        ctr0->iterator().get_or_throw()->dump();

        std::cout << "Props: " << ctr0->ctr_properties() << std::endl;

        ctr0->remove_ctr_property("prop1").throw_if_error();
        ctr0->remove_ctr_property("prop2").throw_if_error();

        ctr0->iterator().get_or_throw()->dump();

        std::cout << "Props: " << ctr0->ctr_properties() << std::endl;

        snp->commit().throw_if_error();
    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cerr);
    }

    return 0;
}
