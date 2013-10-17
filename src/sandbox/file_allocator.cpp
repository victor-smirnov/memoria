
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/memoria.hpp>
#include <memoria/allocators/file/factory.hpp>

using namespace memoria;


Int main() {

	MetadataRepository<FileProfile<> >::init();

	typedef FCtrTF<Vector<Int>>::Type CtrType;

	CtrType::initMetadata();

	try {

		GenericFileAllocator allocator("file.db");

		if (allocator.is_new())
		{
			CtrType ctr(&allocator, CTR_CREATE, 100000);

			vector<Int> data(1000000);

			Int cnt = 0;
			for (auto& a: data)
			{
				a = cnt++;
			}

			ctr.seek(0).insert(data);

			allocator.commit();
		}
		else {
			CtrType ctr(&allocator, CTR_FIND, 100000);

			std::cout<<"Size: "<<ctr.size()<<std::endl;

			auto iter = ctr.seek(0);

			while (iter.nextLeaf())
			{
				iter.dump();
			}
		}
	}
	catch (Exception ex)
	{
		std::cout<<ex.source()<<" "<<ex<<std::endl;
	}

	return 0;
}
