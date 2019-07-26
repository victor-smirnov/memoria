
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
#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/type_registry.hpp>

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/api/map/map_api.hpp>

#include <iostream>


using namespace memoria::v1;

int main()
{
    auto alloc = IMemoryStore<>::create();


    auto snp = alloc->master()->branch();

    using MapType = Map<U8String, U8String>;

    auto ctr = create<MapType>(snp);

    UUID ctr_id = ctr.name();

    ctr.assign("AAAAA", "BBBBB");

    snp->commit();
    snp->set_as_master();

    auto ii = ctr.begin();

    while (!ii.is_end()) {
        std::cout << ii.key() << " = " << ii.value() << std::endl;
        ii.next();
    }


    alloc->store("allocator.mma2");
    auto alloc2 = IMemoryStore<>::load("allocator.mma2");
    auto snp2 = alloc2->master();

    auto ctr2 = find<MapType>(snp2, ctr_id);

    auto ii2 = ctr2.begin();

    while (!ii2.is_end()) {
        std::cout << ii2.key() << " = " << ii2.value() << std::endl;
        ii2.next();
    }

    std::cout << "C deleted" << std::endl;

    std::cout << make_datatype_signature<TimeWithTimeZone>().name() << std::endl;

    std::string text = "Multimap1<Dynamic BigDecimal, BigInt>";

    TypeSignature ts(text);

    DataTypeRegistryStore::global().register_creator_fn<
            Multimap1<Dynamic<BigDecimal>, BigInt>,
            TL<>
    >();

    boost::any obj = DataTypeRegistry::local().create_object(ts.parse());

    std::cout << demangle(obj.type().name()) << std::endl;

    auto objt = boost::any_cast<Multimap1<Dynamic<BigDecimal>, BigInt>>(obj);

    //std::cout << "0Arg: " << objt.key().precision() << " " << objt.key().scale() << std::endl;
    return 0;
}
