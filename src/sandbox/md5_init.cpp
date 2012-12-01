/*
 * partial_specialization.cpp
 *
 *  Created on: 30.11.2012
 *      Author: developer
 */

#include <typeinfo>
#include <iostream>
#include <math.h>
#include <limits.h>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/algo/select.hpp>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;



int main(void) {

    for (int i = 1; i <= 64; i++) {
        cout<<UInt(UINT_MAX * fabs(sin(i)))<<", "<<endl;
    }

    return 0;
}

