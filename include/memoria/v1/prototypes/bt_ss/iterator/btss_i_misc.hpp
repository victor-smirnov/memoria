
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/prototypes/bt_ss/btss_names.hpp>

namespace memoria {
namespace v1 {

using bt::StreamTag;

MEMORIA_ITERATOR_PART_BEGIN(v1::btss::IteratorMiscName)

    using typename Base::NodeBaseG;
    using typename Base::Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using Position = typename Container::Types::Position;
    using CtrSizeT = typename Container::Types::CtrSizeT;
public:
    bool operator++() {
        return self().skipFw(1);
    }

    bool next() {
        return self().skipFw(1);
    }

    bool prev() {
        return self().skipBw(1);
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

    Int size() const
    {
        return self().leafSize(0);
    }

    bool isEof() const {
        return self().idx() >= self().size();
    }

    bool isBof() const {
        return self().idx() < 0;
    }

    CtrSizeT skipFw(CtrSizeT amount) {
        return self().template skip_fw_<0>(amount);
    }

    CtrSizeT skipBw(CtrSizeT amount) {
        return self().template skip_bw_<0>(amount);
    }

    CtrSizeT skip(CtrSizeT amount) {
        return self().template skip_<0>(amount);
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

        Position sizes;

        self.ctr().removeEntries(from_path, from_pos, to_path, to_pos, sizes, true);

        self.idx() = to_pos.get();

        self.refresh();

        return sizes[0];
    }


    auto bulk_insert(btss::AbstractBTSSInputProvider<Container>& provider)
    {
        auto& self = this->self();
        return self.ctr().insert(self, provider);
    }

    template <typename InputIterator>
    auto bulk_insert(InputIterator begin, InputIterator end)
    {
        auto& self = this->self();

        btss::BTSSIteratorInputProvider<Container, InputIterator> provider(self.ctr(), begin, end);

        return self.ctr().insert(self, provider);
    }

    template <typename Iterator>
    class EntryAdaptor {
        Iterator current_;
    public:
        EntryAdaptor(const Iterator& current): current_(current) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<0>, V&& entry) {}

        template <Int SubstreamIdx, typename V>
        void put(StreamTag<0>, StreamTag<SubstreamIdx>, Int block, V&& entry) {
            *current_ = entry;
        }

        void next() {
            current_++;
        }
    };

    template <typename OutputIterator>
    CtrSizeT read(OutputIterator begin, CtrSizeT length)
    {
        auto& self = this->self();

        EntryAdaptor<OutputIterator> adaptor(begin);

        return self.ctr().template read_entries<0>(self, length, adaptor);
    }

protected:

    SplitStatus split()
    {
        auto& self = this->self();

        NodeBaseG& leaf = self.leaf();
        Int& idx        = self.idx();

        Int size        = self.leaf_size(0);
        Int split_idx   = size/2;

        auto right = self.ctr().split_leaf_p(leaf, Position::create(0, split_idx));

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


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(v1::btss::IteratorMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}}