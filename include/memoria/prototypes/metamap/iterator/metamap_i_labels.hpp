
// Copyright Victor Smirnov 2013.
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

#include <memoria/core/packed/map/packed_map.hpp>
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

    static const Int LabelsIndexStart											= Indexes + 1;



    struct LabelRankFn: bt1::NoRtnLeveledNodeWalkerBase<LabelRankFn> {

    	CtrSizeT rank_ = 0;

    	Int label_num_;
    	Int symbol_;

    	Int label_block_offset_;

    	LabelRankFn(Int label_num, Int label_block_offset, Int symbol):
    		label_num_(label_num),
    		symbol_(symbol),
    		label_block_offset_(label_block_offset)
    	{}

        template <Int Idx, typename Stream>
        void leafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->rank(label_num_, idx, symbol_);
        }

        template <Int Idx, typename Stream>
        void nonLeafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->sum(label_block_offset_ + symbol_, idx);
        }
    };

    struct HiddenLabelRankFn: bt1::NoRtnLeveledNodeWalkerBase<HiddenLabelRankFn> {

    	CtrSizeT rank_ = 0;

    	Int label_num_;
    	Int symbol_;

    	Int label_block_offset_;

    	HiddenLabelRankFn(Int label_num, Int label_block_offset, Int symbol):
    		label_num_(label_num),
    		symbol_(symbol),
    		label_block_offset_(label_block_offset)
    	{}

        template <Int Idx, typename Stream>
        void leafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->h_rank(label_num_, idx, symbol_);
        }

        template <Int Idx, typename Stream>
        void nonLeafStream(const Stream* stream, Int idx)
        {
        	rank_ += stream->sum(label_block_offset_ + symbol_, idx);
        }
    };

    CtrSizeT label_rank(Int label_num, Int label) const
    {
    	return p_label_rank<LabelRankFn>(label_num, p_label_block_offset(label_num), label);
    }

    CtrSizeT hidden_label_rank(Int label_num, Int label) const
    {
    	return p_label_rank<HiddenLabelRankFn>(label_num, p_hidden_label_block_offset(label_num), label);
    }


    struct GetLabelFn: bt1::RtnNodeWalkerBase<GetLabelFn> {

    	Int label_num_;
    	GetLabelFn(Int label_num): label_num_(label_num) {}

    	template <Int StreamIdx, typename Stream>
    	Int stream(const Stream* stream, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(stream);
    		return stream->label(idx, label_num_);
    	}
    };

    struct GetHiddenLabelFn: bt1::RtnNodeWalkerBase<GetHiddenLabelFn> {

    	Int label_num_;
    	GetHiddenLabelFn(Int label_num): label_num_(label_num) {}

    	template <Int StreamIdx, typename Stream>
    	Int stream(const Stream* stream, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(stream);
    		return stream->hidden_label(idx, label_num_);
    	}
    };


    Int label(Int label_num) const {
    	return p_label<GetLabelFn>(label_num);
    }

    Int hidden_label(Int label_num) const {
    	return p_label<GetHiddenLabelFn>(label_num);
    }



    struct SetLabelFn: bt1::RtnNodeWalkerBase<SetLabelFn> {

    	Int label_num_;
    	Int new_value_;

    	Int offset_ = 0;

    	SetLabelFn(Int label_num, Int new_value):
    		label_num_(label_num), new_value_(new_value)
    	{}

    	template <Int StreamIdx, typename Stream>
    	Int stream(Stream* stream, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(stream);

    		offset_ = Stream::label_block_offset(label_num_);

    		return stream->set_label(idx, label_num_, new_value_);
    	}
    };

    struct SetHiddenLabelFn: bt1::RtnNodeWalkerBase<SetHiddenLabelFn> {

    	Int label_num_;
    	Int new_value_;

    	Int offset_ = 0;

    	SetHiddenLabelFn(Int label_num, Int new_value):
    		label_num_(label_num), new_value_(new_value)
    	{}

    	template <Int StreamIdx, typename Stream>
    	Int stream(Stream* stream, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(stream);

    		offset_ = Stream::hidden_label_block_offset(label_num_);

    		return stream->set_hidden_label(idx, label_num_, new_value_);
    	}
    };

    Int set_label(Int label_num, Int value)
    {
    	return p_set_label<SetLabelFn, LabelsIndexStart>(label_num, p_label_block_offset(label_num), value);
    }

    Int set_hidden_label(Int label_num, Int value)
    {
    	return p_set_label<SetHiddenLabelFn, LabelsIndexStart>(label_num, p_hidden_label_block_offset(label_num), value);
    }


    CtrSizeT selectLabelFw(Int label_num, Int label, CtrSizeT rank)
    {
    	return p_selectLabelFw(label_num, label, rank, false, p_label_block_offset(label_num));
    }

    CtrSizeT selectHiddenLabelFw(Int label_num, Int label, CtrSizeT rank)
    {
    	return p_selectLabelFw(label_num, label, rank, true, p_hidden_label_block_offset(label_num));
    }

    CtrSizeT selectLabelBw(Int label_num, Int label, CtrSizeT rank)
    {
    	return p_selectLabelBw(label_num, label, rank, false, p_label_block_offset(label_num));
    }

    CtrSizeT selectHiddenLabelBw(Int label_num, Int label, CtrSizeT rank)
    {
    	return p_selectLabelBw(label_num, label, rank, true, p_hidden_label_block_offset(label_num));
    }


    CtrSizeT selectNextLabel(Int label_num, Int label, CtrSizeT rank = 1)
    {
    	Int current_label = self().label(label_num);

    	return self().selectLabelFw(label_num, label, rank + (current_label == label));
    }

    CtrSizeT selectNextHiddenLabel(Int label_num, Int label, CtrSizeT rank = 1)
    {
    	Int current_label = self().hidden_label(label_num);
    	return self().selectHiddenLabelFw(label_num, label, rank + (current_label == label));
    }


private :

    CtrSizeT p_selectLabelFw(Int label_num, Int label, CtrSizeT rank, bool hidden, Int l_offset)
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();
    	Int stream  = self.stream();

    	MEMORIA_ASSERT(rank, >=, 0);

    	typename Types::template SelectForwardWalker<Types> walker(stream, label_num, l_offset + label, label, hidden, rank);

    	walker.prepare(self);

    	Int idx = ctr.findFw(self.leaf(), stream, self.idx(), walker);

    	return walker.finish(self, idx);
    }

    CtrSizeT p_selectLabelBw(Int label_num, Int label, CtrSizeT rank, bool hidden, Int l_offset)
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();
    	Int stream  = self.stream();

    	MEMORIA_ASSERT(rank, >=, 0);

    	typename Types::template SelectBackwardWalker<Types> walker(stream, label_num, l_offset + label, label, hidden, rank);

    	walker.prepare(self);

    	Int idx = ctr.findBw(self.leaf(), stream, self.idx(), walker);

    	return walker.finish(self, idx);
    }


    template <typename Walker>
    CtrSizeT p_label_rank(Int label_num, Int index, Int label) const
    {
    	auto& self = this->self();

    	Walker fn(label_num, index, label);

    	if (self.idx() >= 0)
    	{
    		self.ctr().walkUp(self.leaf(), self.idx(), fn);
    	}

    	return fn.rank_;
    }

    template <typename Walker>
    Int p_label(Int label_num) const
    {
    	auto& self = this->self();
    	return LeafDispatcher::dispatchConstRtn(self.leaf(), Walker(label_num), self.idx());
    }

    template <typename Walker, Int Offset>
    Int p_set_label(Int label_num, Int offset, Int label_value)
    {
    	auto& self = this->self();

    	Walker fn(label_num, label_value);

    	Int old_value = LeafDispatcher::dispatchRtn(self.leaf(), fn, self.idx());

    	Accumulator sums;

    	std::get<0>(sums)[offset + old_value] 		= -1;
    	std::get<0>(sums)[offset + label_value] 	= 1;

    	self.ctr().updateParent(self.leaf(), sums);

    	return old_value;
    }

    Int p_hidden_label_block_offset(Int label_num) const
    {
    	Int offset = Container::Types::HiddenLabelsOffset::offset(label_num);
    	return LabelsIndexStart + offset;
    }

    Int p_label_block_offset(Int label_num) const
    {
    	Int offset = Container::Types::LabelsOffset::offset(label_num);
    	return p_hidden_label_block_offset(HiddenLabels) + offset;
    }

MEMORIA_ITERATOR_PART_END

}

#endif
