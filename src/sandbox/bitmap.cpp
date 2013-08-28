
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <typeinfo>
#include <iostream>


#include <memoria/core/tools/bitmap.hpp>

using namespace std;
using namespace memoria;


int main(void) {

    UBigInt value = 0xff0f0f0f0f0f0f1f;

    cout<<"bitcnt="<<__builtin_popcountl(value)<<" "<<PopCnt((UInt)value)<<endl;

    return 0;
}

