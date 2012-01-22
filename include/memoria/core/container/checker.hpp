
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_CHECKER_HPP
#define	_MEMORIA_CORE_CONTAINER_CHECKER_HPP

//#include <memoria/vapi/models/types.hpp>

#include <stdlib.h>

namespace memoria    {




template <typename ContainerCollectionType, typename AllocatorType>
class Checker {

	typedef typename AllocatorType::PageG				PageG;
	typedef typename AllocatorType::PageG::Page::ID		ID;
	typedef typename AllocatorType::RootMapType 		RootMap;
	typedef typename RootMap::Iterator					RootMapIterator;

	typedef typename ContainerCollectionType::ContainerDispatcherType ContainerDispatcher;

	AllocatorType& allocator_;
	bool result_;

public:
	Checker(AllocatorType& allocator): allocator_(allocator) {}

	// return true in case of errors;
	bool CheckAll()
	{
		ID root = allocator_.root();
		if (root.is_not_null())
		{
			PageG root_page = allocator_.GetPage(root, AllocatorType::READ);
			if (false && dispatch(root_page))
			{
				return true;
			}
			else {
				bool result = false;

				for (RootMapIterator iter = allocator_.roots()->Begin(); !iter.IsEnd(); )
				{
					PageG page = allocator_.GetPage(iter.GetData(), AllocatorType::READ);
					result = dispatch(page) || result;
					iter.Next();
				}

				return result;
			}
		}

		return false;
	}

	bool dispatch(PageG& page) {
		result_ = false;
		ContainerDispatcher::dispatch(&allocator_, page, *this);
		return result_;
	}

	template <typename AllocatorType1, typename Container1>
	void operator()(AllocatorType1& allocator, Container1& model)
	{
		result_ = model.Check(NULL);
	}
};


}

#endif
