
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_ITERATOR_MISC_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_SS_ITERATOR_MISC_HPP

#include <memoria/core/container/macros.hpp>
#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/bt_ss/btss_names.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::btss::IteratorMiscName)

    typedef typename Base::NodeBaseG                                                NodeBaseG;
    typedef typename Base::Container                                                Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::Accumulator                                         Accumulator;
    typedef typename Container::Iterator                                            Iterator;

    using Position = typename Container::Types::Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    bool operator++() {
    	return self().skipFw(1);
    }

    bool operator--() {
    	return self().skipBw(1);
    }

    bool operator++(int) {
    	return self().skipFw(1);
    }

    bool operator--(int) {
    	return self().skipFw(1);
    }

    CtrSizeT operator+=(CtrSizeT size) {
    	return self().skipFw(size);
    }

    CtrSizeT operator-=(CtrSizeT size) {
    	return self().skipBw(size);
	}

    bool isEof() const {
    	return self().idx() >= self().size();
    }

    bool isBof() const {
    	return self().idx() < 0;
    }

    CtrSizeT skipFw(CtrSizeT amount) {
    	return self().template _skipFw<0>(amount);
    }

    CtrSizeT skipBw(CtrSizeT amount) {
    	return self().template _skipBw<0>(amount);
    }

    CtrSizeT skip(CtrSizeT amount) {
    	return self().template _skip<0>(amount);
    }

    CtrSizeT pos() const
    {
    	auto& self = this->self();

    	return self.idx() + self.cache().size_prefix()[0];
    }

    void remove()
    {
        auto& self  = this->self();
        auto& ctr   = self.ctr();

    	ctr.removeEntry(self);

    	if (self.isEnd())
    	{
    		self.skipFw(0);
    	}
    }

    CtrSizeT remove(CtrSizeT size)
    {
        auto& self = this->self();

        auto to = self;

        to.skipFw(size);

        auto& from_path     = self.leaf();
        Position from_pos   = Position(self.idx());

        auto& to_path       = to.leaf();
        Position to_pos     = Position(to.idx());

        Accumulator keys;

        self.ctr().removeEntries(from_path, from_pos, to_path, to_pos, keys, true);

        self.idx() = to_pos.get();

        self.refreshCache();

        return self.ctr().getStreamSizes(keys)[0];
    }


    void refreshCache()
    {
    	auto& self = this->self();
    	self.refresh();
    }

    SplitStatus split()
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

    		self.refresh();

    		return SplitStatus::RIGHT;
    	}
    	else {
    		return SplitStatus::LEFT;
    	}
    }

    template <typename InputProvider>
    CtrSizeT insert(InputProvider& provider)
    {
    	auto& self = this->self();
    	return self.ctr().insert(self, provider);
    }

    template <typename InputIterator>
    CtrSizeT insert(InputIterator begin, InputIterator end)
    {
    	auto& self = this->self();

    	btss::IteratorBTSSInputProvider<Container, InputIterator> provider(self.ctr(), begin, end);

    	return self.insert(provider);
    }

    template <typename OutputIterator>
    CtrSizeT read(OutputIterator begin, CtrSizeT length)
    {
    	auto& self = this->self();
    	return self.ctr().read_entries(self, length, [&](const auto& entry) {
    		*begin = entry;
    		begin++;
    	});
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::btss::IteratorMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}


#endif
