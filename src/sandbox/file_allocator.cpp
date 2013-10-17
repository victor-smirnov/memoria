
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/memoria.hpp>
#include <memoria/allocators/file/factory.hpp>

using namespace memoria;


Int main() {

	MetadataRepository<FileProfile<> >::init();

	try {

		GenericFileAllocator allocator("file.db");

		allocator.commit();
	}
	catch (Exception ex)
	{
		std::cout<<ex.source()<<" "<<ex<<std::endl;
	}

	return 0;
}
