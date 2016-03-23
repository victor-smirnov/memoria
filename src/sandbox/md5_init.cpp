
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <typeinfo>
#include <iostream>
#include <math.h>
#include <limits.h>

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/types/list/append.hpp>
#include <memoria/v1/core/types/algo/select.hpp>

using namespace std;
using namespace memoria;

int main(void) {

    for (int i = 1; i <= 64; i++) {
        cout<<UInt(UINT_MAX * fabs(sin(i)))<<", "<<endl;
    }

    return 0;
}

