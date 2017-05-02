
// Copyright 2014 Victor Smirnov
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



#include <typeinfo>
#include <iostream>

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/core/tools/md5.hpp>
#include <memoria/v1/core/types/typehash.hpp>

using namespace std;
using namespace memoria;

int main(void) {

    uint32_t array[] = {1,3,7,2, 9,2,17,25, 81,43,767,12, 4351,3,7,55,
                    66,77,0,345, 23423,234,45345,45345, 34,23245,2344,56767, 34,2,67,4098,
                    123, 445, 678};

    MD5Hash md5;

    md5.add(sizeof(array)/sizeof(uint32_t));

    for (uint32_t value: array) {
        md5.add(value);
    }

    md5.compute();

    cout<<md5.result().hash64()<<endl;

    cout<<TypeHash<DefaultProfile<>>::Value<<endl;

    return 0;
}
