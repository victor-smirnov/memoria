
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

    DataTypeBuffer<Varchar> str_buffer;

    DataTypeBuffer<Integer> byte_buffer;

    for (int c = 0; c < 5; c++) {
        byte_buffer.builder().value() = 12345 + c;
        byte_buffer.builder().build();
    }

    for (auto ii: byte_buffer.span()) {
        std::cout << ii << std::endl;
    }

    auto& bb = str_buffer.builder();

    bb.append("AAAAAAAAAA111");
    bb.build();

    for (int c = 0; c < 10; c++)
    {
        bb.append("AAAAAAAAAA2222");
        bb.build();
    }


    str_buffer.append("12345");
    str_buffer.append("12323524525246");
    str_buffer.append("124347");
    str_buffer.append("12fgsfdgsfdgsfgsggfsg348");

    for (const auto& str: str_buffer.span()) {
        std::cout << "Str: " << str << std::endl;
    }

//    VLEArenaBuffer<int32_t> vle_buf;

//    std::vector<int32_t> vvv{1,2,3};

//    vvv.push_back(1);

//    vle_buf.append(vvv);

//    for(auto span: vle_buf) {
//        std::cout << span.size() << std::endl;
//    }

    return 0;
}
