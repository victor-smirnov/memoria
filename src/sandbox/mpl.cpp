
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/allocators/persistent-inmem/factory.hpp>

#include <iostream>
#include <string>

using namespace memoria;
using namespace std;

int main() {
	PersistentInMemAllocator<> alloc;

	auto txn = alloc.master();

	txn->commit();

	txn->set_as_master();


//	FSDumpAllocator(&alloc, "pdump.dir");

//	std::string file_name = "store.dump";

//	unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
//	alloc.store(out.get());


//	unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
//	auto alloc2 = PersistentInMemAllocator<>::load(in.get());
}


