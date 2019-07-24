
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

#include <iostream>

using namespace memoria::v1;

int main()
{
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
