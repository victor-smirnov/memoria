
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

	typedef typename Types::IDataSourceType										DataSource;

	BigInt total_size() const
	{
		auto sizes = self().getTotalKeyCount();
		return sizes[1];
	}

	BigInt size() const
	{
		auto sizes = self().getTotalKeyCount();
		return sizes[0];
	}

	BigInt blob_size(Key id) const
	{
		return seek(id).size();
	}

    Iterator seek(Key id, Key pos)
    {
        Iterator iter = self().find(id);
        MEMORIA_ASSERT_TRUE(iter.found());

        iter.findData(pos);

        return iter;
    }

    Iterator find(Key id)
    {
    	auto& self = this->self();

    	vmap::MapFindWalker<Types> walker(id);

    	Iterator iter = self.find0(0, walker);

    	if (iter.id() == id)
    	{
    		iter.found() 	= true;
    		iter.findData(0);
    	}

    	return iter;
    }

    bool contains(Key id) {
    	return find(id).found();
    }

    BigInt maxId()
    {
    	auto& self = this->self();

    	Iterator iter = self.REnd();

    	return iter.id();
    }

    Iterator create(DataSource& src)
    {
    	auto& self = this->self();
    	auto iter  = self.End();

    	self.insert(iter, iter.id() + 1, src);

    	return iter;
    }

    Iterator create(Key id, DataSource& src)
    {
    	auto& self = this->self();
    	auto iter  = self.find(id);

    	if (iter.found())
    	{
    		//self.replaceData(iter, src);
    	}
    	else {
    		self.insert(iter, id, src);
    	}

    	return iter;
    }



MEMORIA_CONTAINER_PART_END

}


#endif
