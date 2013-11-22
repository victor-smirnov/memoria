
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_DBLMAP2_C_API_HPP
#define MEMORIA_CONTAINERS_DBLMAP2_C_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_CONTAINER_PART_BEGIN(memoria::dblmap::CtrApi2Name)

	typedef typename Base::OuterMap                                             OuterMap;
    typedef typename Base::InnerMap                                             InnerMap;

    typedef typename OuterMap::Iterator                                         OuterMapIterator;
    typedef typename InnerMap::Iterator                                         InnerMapIterator;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Types::Key 											Key;

    BigInt size()
    {
    	return self().outer_map().size();
    }

    Iterator find(Key key)
    {
    	auto& self = this->self();
    	auto outer_iter = self.outer_map().find(key);

    	if (!outer_iter.isEnd())
    	{
    		BigInt offset 	= outer_iter.base();
    		auto inner_iter = self.inner_map().seek(offset);

    		inner_iter.resetKeyPrefix();

    		return Iterator(self, outer_iter, inner_iter);
    	}
    	else {
    		auto inner_iter = self.inner_map().End();

    		inner_iter.resetKeyPrefix();

    		return Iterator(self, outer_iter, inner_iter);
    	}
    }

    Iterator create(Key key)
    {
    	auto& self = this->self();

    	auto outer_iter = self.outer_map().findKeyGE(key);

    	if (outer_iter.isEnd() || outer_iter.key() > key)
    	{
    		outer_iter.insert({key, 0});
    		outer_iter--;

    		BigInt offset 	= outer_iter.base();
    		auto inner_iter = self.inner_map().seek(offset);

    		inner_iter.resetKeyPrefix();

    		return Iterator(self, outer_iter, inner_iter);
    	}
    	else {
    		BigInt offset 	= outer_iter.base();
    		auto inner_iter = self.inner_map().seek(offset);

    		inner_iter.resetKeyPrefix();

    		return Iterator(self, outer_iter, inner_iter);
    	}
    }

    Iterator createNew(Key key)
    {
    	auto& self = this->self();

    	auto outer_iter = self.outer_map().findKeyGE(key);

    	if (outer_iter.isEnd() || outer_iter.key() > key)
    	{
    		outer_iter.insert({key, 0});
    		outer_iter--;

    		BigInt offset 	= outer_iter.base();
    		auto inner_iter = self.inner_map().seek(offset);

    		inner_iter.resetKeyPrefix();

    		return Iterator(self, outer_iter, inner_iter);
    	}
    	else {
    		throw vapi::Exception(MA_SRC, SBuf()<<"Requested key "<<key<<" already exists");
    	}
    }

    bool remove(Key key)
    {
    	auto& self = this->self();
    	auto iter = self.find(key);

    	if (iter.is_found_eq(key))
    	{
    		iter.removeEntry();
    		return true;
    	}
    	else {
    		return false;
    	}
    }


    Iterator Begin()
    {
    	auto& self = this->self();

    	auto outer_iter = self.outer_map().Begin();
    	auto inner_iter = self.inner_map().Begin();

    	return Iterator(self, outer_iter, inner_iter);
    }

    Iterator RBegin()
    {
    	auto& self = this->self();

    	auto outer_iter = self.outer_map().RBegin();
    	auto inner_iter = self.inner_map().seek(outer_iter.base());

    	inner_iter.resetKeyPrefix();

    	return Iterator(self, outer_iter, inner_iter);
    }

    Iterator End()
    {
    	auto& self = this->self();

    	auto outer_iter = self.outer_map().End();
    	auto inner_iter = self.inner_map().End();

    	inner_iter.resetKeyPrefix();

    	return Iterator(self, outer_iter, inner_iter);
    }


    Iterator REnd() {
    	return self().Begin();
    }

    void drop()
    {
    	self().inner_map().drop();
    	self().outer_map().drop();
    }


MEMORIA_CONTAINER_PART_END

}


#endif
