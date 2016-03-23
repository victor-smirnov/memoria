
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/v1/tools/tools.hpp>

#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include <random>
#include <functional>

namespace memoria {


void Fill(char* buf, int size, char value)
{
    for (int c  = 0; c < size; c++)
    {
        buf[c] = value;
    }
}





} //memoria
