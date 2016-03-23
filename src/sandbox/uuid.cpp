
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


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
