
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/core/tools/bm_tools.hpp>

#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>


namespace memoria {

using namespace std;

unsigned int m_w = 0x1f23a795;    /* must not be zero */
//unsigned int m_w = 0x3321a795;    /* must not be zero */
unsigned int m_z = 0x351f46be;    /* must not be zero */

void seed(unsigned int value)
{
	m_z += value;
	m_w += value;
}

unsigned int rnd0()
{
    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);
    return (m_z << 16) + m_w;  /* 32-bit result */
}

int get_random(int max) {
    unsigned int val = rnd0();
    return (val & 0x7fffffff) % max;
}

long long getTime() {
    struct timeb tm;
    ftime(&tm);
    return tm.time * 1000 + tm.millitm;
}

int get_number(char *st) {
    char *pEnd;
    int val = (int) strtol(st, &pEnd, 10);
    if ((*pEnd) != 0) {
	return -1;
    }
    return val;
}

int test_pow2(int value) {
    int c, cnt;
    for (c = 0, cnt = 0; c < 32; c++) {
	cnt += (value & (1 << c)) != 0;
    }

    return cnt == 1;
}

} //memoria
