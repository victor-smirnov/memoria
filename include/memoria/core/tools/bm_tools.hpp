
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _BM_TOOLS_H
#define	_BM_TOOLS_H

#include <memoria/core/tools/config.hpp>

#include <stdlib.h>
#include <iostream>

namespace memoria 	{

MEMORIA_API void seed(unsigned int value);
MEMORIA_API int get_random(int max);

MEMORIA_API long long getTime();
MEMORIA_API int get_number(char *st);
MEMORIA_API int test_pow2(int value);

template <typename Task>
class benchmark {
    Task task;
public:
    benchmark(Task &t): task(t) {}

    float run(int times) {
        task.prepare();

        long int t0 = getTime();
        for (int c = 0; c < times; c++) {
            task.calibrate();
        }
        long int t1 = getTime();

        for (int c = 0; c < times; c++) {
            task.run();
        }

        long int t2 = getTime();

        //std::cout<<"times: "<<(t2-t1)<<" "<<(t1-t0)<<std::endl;

        return ((t2 - t1) - (t1 - t0))/(float)times;
    }
};

} //memoria

#endif	/* _BM_TOOLS_H */

