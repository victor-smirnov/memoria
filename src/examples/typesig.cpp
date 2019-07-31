
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
    std::cout << SDNValue::parse("['boooo'@Decimal(5, 2, [1,2,3,4,5]), {'aaa': {'xxxx': ddd }}] @ Long").pretty_print(2) << std::endl;



//    StaticLibraryCtrs<>::init();

    //std::cout << TypeSignature::parse("Real('aaa'@Decimal<Fast>(10,2), '\\\'', 55, 66, [fff, aaa, 'zz', 'ttt' @ Long])").to_standard_string() << std::endl;

    //std::cout << TypeSignature::parse("Real('aaaa\\'bbbb'@Varchar(25))").to_standard_string() << std::endl;

    //U8String txt0 = "Map<Integer, Tensor<Double>(12, 3, 7, 9)>";

//    U8String txt0 = "Map<Decimal(21, 32), U8String>(1,2,34)";

//    DataTypeDeclaration decl = TypeSignature::parse(txt0);

//    std::cout << decl.to_typedecl_string() << std::endl;
//    std::cout << make_datatype_signature<Map<Decimal, U8String>>().name() << std::endl;

//    boost::any dec0 = DataTypeRegistry::local().create_object(decl);

//    auto map0 = boost::any_cast<Map<Decimal, U8String>>(dec0);

//    std::cout << map0.key().precision() << " :: " << map0.key().scale() << std::endl;
/*
    using MapType = Map<U8String, U8String>;

    auto alloc = IMemoryStore<>::create();

    auto snp = alloc->master()->branch();

    //auto ctr1 = create<MapType>(snp);

    auto ctr0 = snp->create_ctr(MapType());

    //auto bbb = snp->create_ctr(TypeSignature::parse("Map<Real, U8String>"));

    UUID ctr_id = ctr0->name();

    ctr0->assign_key("AAAAA", "BBBBB");

    snp->commit();
    snp->set_as_master();

    auto ii = ctr0->iterator();

    while (!ii->is_end())
    {
        std::cout << ii->key() << " = " << ii->value() << std::endl;
        ii->next();
    }

    alloc->store("allocator.mma2");
    auto alloc2 = IMemoryStore<>::load("allocator.mma2");
    auto snp2 = alloc2->master();

    auto ctr2 = snp2->find_ctr(MapType(), ctr_id);

    auto ii2 = ctr2->iterator();

    while (!ii2->is_end()) {
        std::cout << ii2->key() << " = " << ii2->value() << std::endl;
        ii2->next();
    }

    std::cout << make_datatype_signature<TimeWithTimeZone>().name() << std::endl;

    std::string text = "Multimap1<Dynamic BigDecimal, BigInt>";
    TypeSignature ts(text);

    std::string text2 = "BooType(aaa, 'bbb', 1234, 5.6789, [1, 2, 3, 4, 5, 'boo', foo, _])";
    std::cout << TypeSignature::parse(text2).to_standard_string() << std::endl;

    DataTypeRegistryStore::global().register_creator_fn<
        Multimap1<Dynamic<BigDecimal>, BigInt>,
        TL<>
    >();

    boost::any obj = DataTypeRegistry::local().create_object(ts.parse());

    std::cout << demangle(obj.type().name()) << std::endl;

    boost::any_cast<Multimap1<Dynamic<BigDecimal>, BigInt>>(obj);

    //std::cout << "0Arg: " << objt.key().precision() << " " << objt.key().scale() << std::endl;
*/
  return 0;
}
