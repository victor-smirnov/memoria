/*
 * partial_specialization.cpp
 *
 *  Created on: 30.11.2012
 *      Author: developer
 */

#include <typeinfo>
#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/md5.hpp>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;

int main(void) {

    UInt array[] = {1,3,7,2, 9,2,17,25, 81,43,767,12, 4351,3,7,55,
                    66,77,0,345, 23423,234,45345,45345, 34,23245,2344,56767, 34,2,67,4098,
                    123, 445, 678};

    MD5Hash md5;

    md5.add(sizeof(array)/sizeof(UInt));

    for (UInt value: array) {
        md5.add(value);
    }

    md5.compute();

    cout<<md5.result().hash64()<<endl;

    return 0;
}

