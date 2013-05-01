
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTORMAP2_C_API_HPP
#define _MEMORIA_CONTAINER_VECTORMAP2_C_API_HPP


#include <memoria/containers/vector_map/vectormap_names.hpp>
#include <memoria/containers/vector_map/vectormap_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::vmap::CtrApiName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::TreeNodePage                                         TreeNodePage;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::NodeDispatcher                                       NodeDispatcher;
	typedef typename Base::RootDispatcher                                       RootDispatcher;
	typedef typename Base::LeafDispatcher                                       LeafDispatcher;
	typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;
	typedef typename Base::Element                                              Element;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;


	BigInt totalSize() const {
		return self().getSize();
	}

	BigInt size(Key id) const
	{
		return seek(id).size();
	}

    Iterator seek(Key id, Key pos)
    {
        Iterator iter = self().find(pos);
        MEMORIA_ASSERT_TRUE(iter.found());

        iter.findData();
        iter.seek(pos);
    }

    Iterator find(Key id)
    {
    	auto& self = this->self();

    	Iterator iter = self.findLE(0, id, 0);

    	if (iter.id() == id)
    	{
    		iter.found() 	= true;
    		iter.lobSize()	= iter.mapEntry(0);

    		iter.findData();
    	}

    	return iter;
    }

    Iterator create(Key id)
    {
    	return Iterator(self());
    }


MEMORIA_CONTAINER_PART_END

}


#endif
