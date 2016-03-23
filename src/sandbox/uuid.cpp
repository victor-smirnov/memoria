
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

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

using namespace std;
using namespace memoria;

int main(void) {

    auto t0 = getTimeInMillis();

    for (Int c = 0; c < 10; c++) {
        cout<<UUID::make_random()<<endl;
    }

    auto t1 = getTimeInMillis();

    cout<<FormatTime(t1 - t0)<<endl;

    return 0;
}
