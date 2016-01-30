
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/allocators/persistent-inmem/factory.hpp>

#include <iostream>

using namespace memoria;
using namespace std;

int main() {
	PersistentInMemAllocator<> alloc;

	auto txn = alloc.master();

	FSDumpAllocator(&alloc, "pdump.dir");
}


