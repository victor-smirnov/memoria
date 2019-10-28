
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

#include <memoria/v1/core/sdn/sdn2.hpp>

#include <memoria/v1/core/tools/uuid.hpp>

using namespace memoria::v1;

int main()
{
//    SDN2Document doc;

//    SDN2Map map = doc.set_map();
//    map.set("key0", "value0");
//    map.set("key1", "value1");
//    map.set("key2", 12345.6789);

//    SDN2Array arr = map.add_array("key3");
//    arr.add("cool value");
//    arr.add((int64_t)1234);
//    arr.add(1234.567);
//    arr.add_array().add(567.99);
//    arr.add_map();

//    doc.dump(std::cout);
//    std::cout << std::endl;

    SDN2Document doc = SDN2Document::parse("{a: b, c: [7.891, 1, {}]}");
    doc.dump(std::cout);

    return 0;
}
