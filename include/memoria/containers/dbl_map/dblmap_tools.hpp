
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP_TOOLS_HPP
#define _MEMORIA_CONTAINERS_DBLMAP_TOOLS_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/idata.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/core/packed/map/packed_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <tuple>

namespace memoria       {
namespace dblmap        {




typedef std::pair<BigInt, BigInt> VectorMapEntry;


class VectorMapSource: public ISource {

    IDataBase* sources_[2];

    EmptyDataSource<VectorMapEntry> map_source_;

public:
    VectorMapSource(IDataBase* source)
    {
        sources_[0] = &map_source_;
        sources_[1] = source;
    }

    virtual Int streams()
    {
        return 2;
    }

    virtual IData* stream(Int stream)
    {
        return sources_[stream];
    }

    virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes)
    {
        Int allocated[2] = {0, 0};
        Int capacity = layout_manager->getNodeCapacity(allocated, 1);

        sizes[1] = capacity;
    }

    virtual BigInt getTotalNodes(INodeLayoutManager* manager)
    {
        Int sizes[2] = {0, 0};

        SizeT capacity  = manager->getNodeCapacity(sizes, 1);
        SizeT remainder = sources_[1]->getRemainder();

        return remainder / capacity + (remainder % capacity ? 1 : 0);
    }
};


class VectorMapTarget: public ITarget {

    IDataBase* targets_[2];

    EmptyDataTarget<VectorMapEntry> map_target_;
public:
    VectorMapTarget(IDataBase* target)
    {
        targets_[0] = &map_target_;
        targets_[1] = target;
    }

    virtual Int streams()
    {
        return 2;
    }

    virtual IData* stream(Int stream)
    {
        return targets_[stream];
    }
};


template <typename Iterator, typename Container>
class DblMapIteratorPrefixCache: public bt::BTreeIteratorCache<Iterator, Container> {

    typedef bt::BTreeIteratorCache<Iterator, Container>                         Base;
    typedef typename Container::Accumulator                                     Accumulator;

    BigInt id_prefix_   = 0;
    BigInt id_entry_    = 0;
    BigInt size_        = 0;
    BigInt base_        = 0;

    BigInt second_prefix_ = 0;

    BigInt global_pos_  = 0;

    Int entry_idx_      = 0;
    Int entries_        = 0;

public:

    DblMapIteratorPrefixCache(): Base() {}


    BigInt id() const
    {
        return id_prefix_ + id_entry_;
    }

    BigInt id_prefix() const
    {
        return id_prefix_;
    }

    BigInt id_entry() const
    {
        return id_entry_;
    }

    BigInt size() const
    {
        return size_;
    }

    BigInt blob_base() const
    {
        return base_;
    }

    Int entry_idx() const
    {
        return entry_idx_;
    }

    void setEntryIdx(Int entry_idx)
    {
        entry_idx_ = entry_idx;
    }

    void addEntryIdx(Int entry_idx)
    {
        entry_idx_ += entry_idx;
    }

    Int entries() const {
        return entries_;
    }

    void setEntries(Int entries)
    {
        entries_ = entries;
    }


    BigInt global_pos() const {
        return global_pos_;
    }

    void addToGlobalPos(BigInt value) {
        global_pos_ += value;
    }

    void set_positions(BigInt global_pos)
    {
        global_pos_ = global_pos;
    }

    void addToEntry(BigInt entry, BigInt size) {
        id_entry_   += entry;
        size_       += size;

        if (entry > 0)
        {
            second_prefix_ = 0;
        }
    }


    void setup(
            BigInt id_prefix,
            BigInt id_entry,
            BigInt base,
            BigInt size,
            Int entry_idx,
            Int entries,
            BigInt global_pos
        )
    {
        id_prefix_  = id_prefix;
        id_entry_   = id_entry;

        size_       = size;
        base_       = base;

        entry_idx_  = entry_idx;
        entries_    = entries;

        global_pos_ = global_pos;
    }

    void add(BigInt id_entry, BigInt size, Int entry_idx)
    {
        id_prefix_  += id_entry_;
        base_       += size_;

        id_entry_   = id_entry;
        size_       = size;

        entry_idx_  = entry_idx;

        second_prefix_ = 0;
    }

    void sub(BigInt id_entry, BigInt size, Int entry_idx)
    {
        id_prefix_  -= id_entry;
        base_       -= size;

        id_entry_   = id_entry;
        size_       = size;

        entry_idx_  = entry_idx;

        second_prefix_ = 0;
    }

    void set(BigInt id_entry, BigInt size, Int entry_idx, Int entries, BigInt global_pos)
    {
        id_entry_   = id_entry;
        size_       = size;

        entry_idx_  = entry_idx;
        entries_    = entries;

        global_pos_ = global_pos;

        second_prefix_ = 0;
    }

    BigInt& second_prefix() {
        return second_prefix_;
    }

    const BigInt& second_prefix() const {
        return second_prefix_;
    }
};






template <typename Iterator, typename Container>
class OuterMapIteratorPrefixCache: public bt::BTreeIteratorCache<Iterator, Container> {
    typedef bt::BTreeIteratorCache<Iterator, Container> Base;

    typedef typename Container::Accumulator                                     Accumulator;

    Accumulator prefix_;
    Accumulator current_;

public:

    OuterMapIteratorPrefixCache(): Base(), prefix_(), current_() {}

    const BigInt& prefix(int num = 0) const
    {
        return std::get<0>(prefix_)[num];
    }

    const Accumulator& current() const
    {
        return current_;
    }

    const Accumulator& prefixes() const
    {
        return prefix_;
    }

    Accumulator& prefixes()
    {
        return prefix_;
    }

    void nextKey(bool end)
    {
        VectorAdd(prefix_, current_);

        Clear(current_);
    };

    void prevKey(bool start)
    {
        VectorSub(prefix_, current_);

        Clear(current_);
    };

    void setup(const Accumulator& prefix)
    {
        prefix_ = prefix;
    }

    void Clear(Accumulator& v) const
    {
        std::get<0>(v).clear();
    }

    void Prepare()
    {
        if (Base::iterator().idx() >= 0)
        {
            current_ = Base::iterator().entry();
        }
        else {
            Clear(current_);
        }
    }
};



template <typename Iterator, typename Container>
class InnerMapIteratorPrefixCache: public bt::BTreeIteratorCache<Iterator, Container> {
    typedef bt::BTreeIteratorCache<Iterator, Container> Base;

    typedef typename Container::Accumulator                                     Accumulator;

    Accumulator prefix_;

public:

    InnerMapIteratorPrefixCache(): Base() {}

    const Accumulator& prefixes() const
    {
        return prefix_;
    }

    Accumulator& prefixes()
    {
        return prefix_;
    }

    void setup(const Accumulator& prefix)
    {
        prefix_ = prefix;
    }
};




}
}

#endif
