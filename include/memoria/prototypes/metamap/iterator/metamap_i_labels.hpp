
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_LABELS_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_LABELS_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_tools.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrLabelsName)

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
    typedef typename Container::Types::CtrSizeT                           		CtrSizeT;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;

    static const Int Indexes													= Container::Types::Indexes;
    static const Int Labels														= Container::Types::Labels;
    static const Int HiddenLabels												= Container::Types::HiddenLabels;

    static const Int HiddenLabelsOffset											= Container::Types::HiddenLabelsOffset;
    static const Int LabelsOffset												= Container::Types::LabelsOffset;

    struct LabelRankFn: bt1::UpWalkerBase<LabelRankFn> {

    	CtrSizeT rank_ = 0;
    	Int symbol_;

    	LabelRankFn(Int symbol): symbol_(symbol) {}

        template <Int Idx, typename Stream>
        void leafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->rank(idx, symbol_);
        }

        template <Int Idx, typename Stream>
        void nonLeafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->sum(LabelsOffset + symbol_, idx);
        }
    };

    struct HiddenLabelRankFn: bt1::UpWalkerBase<HiddenLabelRankFn> {

    	CtrSizeT rank_ = 0;
    	Int symbol_;

    	HiddenLabelRankFn(Int symbol): symbol_(symbol) {}

        template <Int Idx, typename Stream>
        void leafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->h_rank(idx, symbol_);
        }

        template <Int Idx, typename Stream>
        void nonLeafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->sum(HiddenLabelsOffset + symbol_, idx);
        }
    };

    CtrSizeT label_rank(Int label) const
    {
    	auto& self = this->self();

    	LabelRankFn fn(label);

    	if (self.idx() >= 0)
    	{
    		self.ctr().walkUp(self.leaf(), self.idx(), fn);
    	}

    	return fn.rank_;
    }

    CtrSizeT hidden_label_rank(Int label) const
    {
    	auto& self = this->self();

    	HiddenLabelRankFn fn(label);

    	if (self.idx() >= 0)
    	{
    		self.ctr().walkUp(self.leaf(), self.idx(), fn);
    	}

    	return fn.rank_;
    }





    CtrSizeT selectLabelFw(Int label, CtrSizeT rank)
    {
    	return p_selectLabelFw(label, rank, false, LabelsOffset);
    }

    CtrSizeT selectHiddenLabelFw(Int label, CtrSizeT rank)
    {
    	return p_selectLabelFw(label, rank, true, HiddenLabelsOffset);
    }

    CtrSizeT selectLabelBw(Int label, CtrSizeT rank)
    {
    	return p_selectLabelBw(label, rank, false, LabelsOffset);
    }

    CtrSizeT selectHiddenLabelBw(Int label, CtrSizeT rank)
    {
    	return p_selectLabelBw(label, rank, true, HiddenLabelsOffset);
    }




private :

    CtrSizeT p_selectLabelFw(Int label, CtrSizeT rank, bool hidden, Int label_offset)
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();
    	Int stream  = self.stream();

    	MEMORIA_ASSERT(rank, >=, 0);

    	typename Types::template SelectForwardWalker<Types> walker(stream, label_offset + label, label, hidden, rank);

    	walker.prepare(self);

    	Int idx = ctr.findFw(self.leaf(), stream, self.idx(), walker);

    	return walker.finish(self, idx);
    }

    CtrSizeT p_selectLabelBw(Int label, CtrSizeT rank, bool hidden, Int label_offset)
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();
    	Int stream  = self.stream();

    	MEMORIA_ASSERT(rank, >=, 0);

    	typename Types::template SelectBackwardWalker<Types> walker(stream, label_offset + label, label, hidden, rank);

    	walker.prepare(self);

    	Int idx = ctr.findBw(self.leaf(), stream, self.idx(), walker);

    	return walker.finish(self, idx);
    }

MEMORIA_ITERATOR_PART_END

}

#endif
