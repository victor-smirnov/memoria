
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/typelist.hpp>

#include <iostream>
#include <functional>
#include <typeinfo>

#include <math.h>

using namespace std;
using namespace memoria;



int main(void) {


	ListPrinter<TL<
		decltype(&function<Int(Int, Int)>::operator())
	>>::print();

	cout<<sizeof(decltype(&function<Int(Int, Int)>::operator()))<<endl;

    return 0;
}
