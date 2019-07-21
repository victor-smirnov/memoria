
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


#include <iostream>

using namespace memoria::v1;

int main() {

    //std::string text = "Real type <a b c d, b(1, 2, bar)>(1, 2, foo, '4', 5.1234e6)   ";
    //std::string text = "Real dd<bbbbb<>, ccc(1)>(abc, 111,       222, 'BOOO!')";
    std::string text = "Real<aaa<yy>(1234, 5593133453452345.66778899)  ,  bb(5678,'aaa')>()";

    TypeSignature ts(text);

    DataTypeDeclaration decl = ts.parse(text);

    auto s1 = decl.to_standard_string();

    TypeSignature ts2(s1);

    auto s2 = ts2.parse().to_standard_string();


    std::cout << s1 << std::endl;
    std::cout << s2 << std::endl;

    return 0;
}
