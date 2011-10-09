
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

	typedef typename AllocatorType::Page		Page;
	typedef typename AllocatorType::Page::ID	ID;
	typedef typename AllocatorType::RootMapType RootMap;
	typedef typename RootMap::Iterator			RootMapIterator;

	typedef typename ContainerCollectionType::ContainerDispatcherType ContainerDispatcher;

	AllocatorType& allocator_;
	bool result_;

public:
	Checker(AllocatorType& allocator): allocator_(allocator) {}

	bool CheckAll()
	{
		ID root = allocator_.root();
		if (root.is_not_null())
		{
			Page* root_page = allocator_.GetPage(root);
			if (false && dispatch(root_page))
			{
				return true;
			}
			else {
				bool result = false;

				RootMapIterator iter = allocator_.roots()->Begin();
				RootMapIterator end = allocator_.roots()->End();

				while (iter != end)
				{
					Page* page = allocator_.GetPage(iter.GetData());
					result = dispatch(page) || result;
					iter.Next();
				}

				return result;
			}
		}

		return false;
	}

	bool dispatch(Page* page) {
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
