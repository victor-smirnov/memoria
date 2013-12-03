
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_MISC_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_MISC_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_tools.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrMiscName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;
    typedef typename Container::Position                                        Position;

    typedef typename Container::Types::IteratorPrefix                           IteratorPrefix;
    typedef typename Container::Types::CtrSizeT									CtrSizeT;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;

    void updateUp(Int index, CtrSizeT delta)
    {
    	auto& self 	= this->self();
    	auto& leaf	= self.leaf();
    	auto& idx	= self.idx();

    	self.ctr().updateUp(leaf, idx, bt::SingleIndexUpdateData<CtrSizeT>(0, index, delta), [&](Int, Int _idx) {
    		idx = _idx;
    		self.updatePrefix();
    	});
    }


    struct PrefixFn: bt1::NoRtnLeveledNodeWalkerBase<PrefixFn> {
    	IteratorPrefix prefix_;

    	PrefixFn() {}

    	template <Int Idx, typename Stream>
    	void leafStream(const Stream* stream, Int idx)
    	{
    		if (stream)
    		{
    			std::get<0>(prefix_)[0] += idx;

    			for (Int c = 1; c < std::tuple_element<0, IteratorPrefix>::type::Indexes; c++)
    			{
    				std::get<0>(prefix_)[c] += stream->sum(c - 1, idx);
    			}
    		}
    	}

    	template <Int Idx, typename Stream>
    	void nonLeafStream(const Stream* stream, Int idx)
    	{
    		for (Int c = 9; c < std::tuple_element<0, IteratorPrefix>::type::Indexes; c++)
    		{
    			std::get<0>(prefix_)[c] += stream->sum(c, idx);
    		}
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

    	self.cache().prefixes() = fn.prefix_;
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




    void ComputePrefix(BigInt& v)
    {
    	Accumulator accum;
    	ComputePrefix(accum);

    	v = std::get<0>(accum)[1];
    }

    void ComputePrefix(Accumulator& accum)
    {
    	auto& self = this->self();

    	PrefixFn fn;

    	if (self.idx() >= 0)
    	{
    		self.ctr().walkUp(self.leaf(), self.idx(), fn);
    	}

    	std::get<0>(accum).sumAt(0, std::get<0>(fn.prefix_));
    }

    void dump(std::ostream& out = std::cout)
    {
    	out<<"Prefixes="<<self().cache().prefixes()<<endl;
    	Base::dump(out);
    }
MEMORIA_ITERATOR_PART_END

}

#endif
