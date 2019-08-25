
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

#include <iostream>

using namespace memoria::v1;

struct TypeData;
struct Digits;

struct DecimalView {
    TypeData* ext_data_;
    Digits* disgits_;

    bool is_zero() const {
        //....
        return true; // or false
    }

    //....
    //....
    //....
};

using DecimalBuf = Span<DecimalView>;

int main()
{
    AnyDatum uuid = DataTypeRegistry::local().from_sdn_string("'c7c9c4ef-cd29-4028-aee5-1832cca9ff0f'@UUID");

    std::cout << uuid << std::endl;

    std::cout << datum_cast<UUID>(std::move(uuid)).view() << std::endl;

    //Datum<Decimal> dt;

    //DataTypeTraits<Deciaml>::ViewType -> DecimalView

    return 0;
}
