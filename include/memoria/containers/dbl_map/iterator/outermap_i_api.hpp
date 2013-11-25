
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_DBLMAP2_OUTER_I_API_HPP
#define _MEMORIA_CONTAINER_DBLMAP2_OUTER_I_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>
#include <memoria/containers/dbl_map/dblmap_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>
#include <tuple>

namespace memoria    {

MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::dblmap::OuterItrApiName)

    typedef Ctr<typename Types::CtrTypes>                       				Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Key                                             Key;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    typedef	typename std::tuple_element<0, Accumulator>::type					Entry0;

    typedef typename Container::CtrSizeT                        				CtrSizeT;



    void updateUp(const Accumulator& keys)
    {
    	auto& self = this->self();

    	self.ctr().updateUp(self.leaf(), self.idx(), keys, [&](Int, Int idx) {
    		self.idx() = idx;
    		self.updatePrefix();
    	});
    }

    Accumulator prefixes() const {
    	return self().cache().prefixes();
    }


	struct GetEntryFn {
		Accumulator element_;

		GetEntryFn() {}

		template <Int Idx, typename StreamTypes>
		void stream(const PkdFTree<StreamTypes>* map, Int idx)
		{
			MEMORIA_ASSERT_TRUE(map);
			MEMORIA_ASSERT(idx, <, map->size());

			map->sums(idx, idx + 1, std::get<Idx>(element_));
		}


		template <typename NTypes>
		void treeNode(const LeafNode<NTypes>* node, Int idx)
		{
			node->template processStream<0>(*this, idx);
		}
	};


    Accumulator entry() const
    {
    	auto& self = this->self();

    	GetEntryFn fn;

    	LeafDispatcher::dispatchConst(self.leaf(), fn, self.idx());

    	return fn.element_;
    }

    CtrSizeT size() const {
    	return std::get<0>(entry())[1];
    }

    CtrSizeT pos() const
    {
    	return std::get<0>(self().cache().prefixes())[0];
    }

    CtrSizeT base() const
    {
    	auto& self = this->self();
    	return std::get<0>(self.cache().prefixes())[1];
    }

    BigInt key() const
    {
    	auto& self = this->self();
    	return std::get<0>(self.entry())[0] + std::get<0>(self.cache().prefixes())[0];
    }


    void addSize(BigInt delta)
    {
    	auto& self = this->self();

    	Accumulator sums;
    	std::get<0>(sums)[1] = delta;

    	self.updateUp(sums);
    }

    void insert(const Entry0& element)
    {
    	auto& self = this->self();
    	self.ctr().insert(self, element);
    }

    void remove()
    {
    	auto& self = this->self();
    	self.ctr().remove(self);
    }


    struct PrefixFn {
    	Accumulator prefix_;

    	PrefixFn() {}

    	template <Int Idx, typename StreamTypes>
    	void stream(const PkdFTree<StreamTypes>* map, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(map);

    		map->sums(0, idx, std::get<Idx>(prefix_));
    	}

    	template <typename Node>
    	void treeNode(const Node* node, Int idx)
    	{
    		node->template processStream<0>(*this, idx);
    	}
    };

    void updatePrefix()
    {
    	auto& self = this->self();

    	PrefixFn fn;

    	if (self.idx() >= 0)
    	{
    		self.ctr().walkUp(self.leaf(), self.idx(), fn);
    	}

    	self.cache().setup(fn.prefix_);
    }

    void split()
    {
    	auto& self = this->self();

    	NodeBaseG& leaf = self.leaf();
    	Int& idx        = self.idx();

    	Int size        = self.leaf_size(0);
    	Int split_idx   = size/2;

    	auto right = self.ctr().splitLeafP(leaf, Position::create(0, split_idx));

    	if (idx > split_idx)
    	{
    		leaf = right;
    		idx -= split_idx;

    		self.updatePrefix();
    	}
    }

    void dump() const {
        auto cache = self().cache();

        cout<<"Cache: prefixes=" <<cache.prefixes()<<endl;

        Base::dump();
    }

    bool is_found_eq(Key key) const
    {
    	auto& self = this->self();
    	return (!self.isEnd()) && self.key() == key;
    }

    bool is_found_le(Key key) const
    {
    	auto& self = this->self();

    	return self.isContent() && self.key() <= key;
    }

    bool is_found_lt(Key key) const
    {
    	auto& self = this->self();
    	return self.isContent() && self.key() <= key;
    }

    bool is_found_ge(Key key) const
    {
    	auto& self = this->self();
    	return self.isContent() && self.key() >= key;
    }

    bool is_found_gt(Key key) const
    {
    	auto& self = this->self();
    	return self.isContent() && self.key() > key;
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::dblmap::OuterItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS
}

#endif
