// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/core/tools/type_name.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>
#include <unordered_map>

#include <math.h>
#include <iostream>
#include <iomanip>

#include <limits>

using namespace std;

template <typename R, typename T>
R& bit_cast(T& pX)
{
    return reinterpret_cast<R&>(pX);
}


int main ()
{
    float f = numeric_limits<float>::min();
    int last_int;

    cout<<"Start f: "<<f<<endl;

    int i, int_f;

    for(i = numeric_limits<int>::min(); i < numeric_limits<int>::max(); ++i)
    {
    	int_f = bit_cast<long long>(f);

//    	if (int_f - last_int != 1)
//    	{
////    		cout<<"Exception "<<f<<" "<<i<<" "<<int_f<<" "<<last_int<<endl;
////    		break;
//    	}
//    	else
    	{
    		last_int = int_f;
    	}

    	if (i % 10000000 == 0) {
    		cout<<i<<endl;
    	}

    	//std::cout << std::setprecision(70) << f <<" "<<bit_cast<long long>(f)<< std::endl;

    	f = nexttowardf(f, numeric_limits<float>::max());
    }

    cout<<f<<" "<<i<<" "<<int_f<<" "<<last_int<<endl;
  return 0;
}
