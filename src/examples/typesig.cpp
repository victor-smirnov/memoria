
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


#include <memoria/v1/api/datatypes/type_signature.hpp>
#include <memoria/v1/api/datatypes/sdn.hpp>
#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/type_registry.hpp>

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/api/map/map_api.hpp>

#include <memoria/v1/memoria.hpp>

#include <iostream>

using namespace memoria::v1;

int main()
{
    std::cout << TypeSignature::parse(R"(Root("boo"))").to_standard_string() << std::endl;

    StaticLibraryCtrs<>::init();

    using MapType = Map<Varchar, Varchar>;

    auto alloc = IMemoryStore<>::create();

    auto snp = alloc->master()->branch();

    auto ctr0 = snp->create_ctr(MapType());

    UUID ctr_id = ctr0->name();

    for (int c = 0; c < 10000; c++) {
        ctr0->assign_key("AAAAA_" + std::to_string(c), "BBBBB_" + std::to_string(c));
    }

    snp->commit();
    snp->set_as_master();

    auto ii = ctr0->scanner();

    while (!ii.is_end())
    {
        for (size_t c = 0; c < ii.keys().size(); c++) {
            std::cout << ii.keys()[c] << " = " << ii.values()[c] << std::endl;
        }

        ii.next_leaf();
    }



    return 0;
}
