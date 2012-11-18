
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PMAP_PACKED_SUM_TREE_HPP_
#define MEMORIA_CORE_PMAP_PACKED_SUM_TREE_HPP_


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/traits.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/pmap/packed_tree.hpp>
#include <memoria/core/pmap/tree_walkers.hpp>

namespace memoria {

template <typename Types>
class PackedSumTree: public PackedTree<Types> {

    typedef PackedTree<Types>               Base;
    typedef PackedSumTree<Types>            MyType;

public:

    typedef typename Base::Key              Key;
    typedef typename Base::IndexKey         IndexKey;
    typedef typename Base::Value            Value;
    typedef typename Base::Accumulator      Accumulator;

    static const Int Blocks                 = Base::Blocks;
    static const Int BranchingFactor        = Base::BranchingFactor;

private:


public:
    PackedSumTree(): Base() {}

    Value& data(Int idx) {
        return Base::value(idx);
    }

    const Value& data(Int idx) const {
        return Base::value(idx);
    }

    void reindex()
    {
        for (Int c = 0; c < Blocks; c++)
        {
            reindex(c, 0, Base::size());
        }
    }


    void reindex(Int block_num)
    {
        reindex(block_num, 0, Base::size());
    }

    void reindexAll(Int start, Int end)
    {
        for (Int c = 0; c < Blocks; c++)
        {
            reindex(c, start, end);
        }
    }

    void updateUp(Int block_num, Int idx, IndexKey key_value)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, Base::size());

        Base::key(block_num, idx) += key_value;

        Int level_size      = Base::maxSize();
        Int level_start     = Base::indexSize();

        Int block_offset    = Base::getIndexKeyBlockOffset(block_num);

        do {
            level_size      = Base::getIndexCellsNumberFor(level_size);
            level_start     -= level_size;

            idx /= BranchingFactor;

            Base::indexb(block_offset, idx + level_start) += key_value;
        }
        while (level_start > 0);
    }

    void reindex(Int block_num, Int start, Int end)
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, <=, Base::size());
        MEMORIA_ASSERT(start, <=, end);

        Int block_start = Base::getBlockStart(start);
        Int block_end   = Base::getBlockEnd(end);

        Int index_block_offset  = Base::getIndexKeyBlockOffset(block_num);
        Int key_block_offset    = Base::getKeyBlockOffset(block_num);

        Int index_level_size    = Base::getIndexCellsNumberFor(Base::maxSize());
        Int index_level_start   = Base::indexSize() - index_level_size;

        Int level_max           = Base::size();

        for (Int c = block_start; c < block_end; c += BranchingFactor)
        {
            IndexKey sum = 0;
            Int max      = c + BranchingFactor <= level_max ? c + BranchingFactor : level_max;

            for (Int d = c; d < max; d++)
            {
                sum += Base::keyb(key_block_offset, d);
            }

            Int idx = c / BranchingFactor + index_level_start;
            Base::indexb(index_block_offset, idx) = sum;
        }

        while (index_level_start > 0)
        {
            level_max       = Base::getIndexCellsNumberFor(level_max);
            block_start     = Base::getBlockStart(block_start / BranchingFactor);
            block_end       = Base::getBlockEnd(block_end / BranchingFactor);

            Int index_parent_size   = Base::getIndexCellsNumberFor(index_level_size);
            Int index_parent_start  = index_level_start - index_parent_size;

            for (Int c = block_start; c < block_end; c += BranchingFactor)
            {
                IndexKey sum = 0;
                Int max      = (c + BranchingFactor <= level_max ? c + BranchingFactor : level_max) + index_level_start;

                for (Int d = c + index_level_start; d < max; d++)
                {
                    sum += Base::indexb(index_block_offset, d);
                }

                Int idx = c / BranchingFactor + index_parent_start;
                Base::indexb(index_block_offset, idx) = sum;
            }

            index_level_size    = index_parent_size;
            index_level_start   -= index_parent_size;
        }
    }


    Int findLE(Int block_num, const Key& k) const
    {
        LESumComparator<Key, IndexKey> cmp;
        return Base::find(block_num, k, cmp);
    }

    //FIXME: Refactor it
    Int findLES(Int block_num, const Key& k, Key& sum) const
    {
        LESumComparator<Key, IndexKey> cmp;
        Int idx = Base::find(block_num, k, cmp);

        if (idx >= 0)
        {
            //FIXME: what does it mean here "size() > 0 ? key(i, idx) : 0" ???
            sum += cmp.sum() - (Base::size() > 0 ? Base::key(block_num, idx) : 0);
        }

        return idx;
    }


    Int findLE(Int block_num, const Key& k, Accumulator& acc) const
    {
        LESumComparator<Key, IndexKey> cmp;
        Int result = Base::find(block_num, k, cmp);
        acc[block_num] += cmp.sum();
        return result;
    }

    Int findLT(Int block_num, const Key& k) const
    {
        LTSumComparator<Key, IndexKey> cmp;
        return Base::find(block_num, k, cmp);
    }

    //FIXME: Refactor it
    Int findLTS(Int block_num, const Key& k, Key& sum) const
    {
        LTSumComparator<Key, IndexKey> cmp;
        Int idx = Base::find(block_num, k, cmp);

        if (idx >= 0)
        {
            //FIXME: what does it mean here "size() > 0 ? key(i, idx) : 0" ???
            sum += cmp.sum() - (Base::size() > 0 ? Base::key(block_num, idx) : 0);
        }

        return idx;
    }

    Int findLT(Int block_num, const Key& k, Accumulator& acc) const
    {
        LTSumComparator<Key, IndexKey> cmp;
        Int result = Base::find(block_num, k, cmp);
        acc[block_num] += cmp.sum();
        return result;
    }


    Int findEQ(Int block_num, const Key& k) const
    {
        EQSumComparator<Key, IndexKey> cmp;
        return Base::find(block_num, k, cmp);
    }

    Int findEQ(Int block_num, const Key& k, Accumulator& acc) const
    {
        EQSumComparator<Key, IndexKey> cmp;
        Int result = Base::find(block_num, k, cmp);
        acc[block_num] += cmp.sum();
        return result;
    }


    void sum(Int block_num, Int start, Int end, IndexKey& sum) const
    {
        SumWalker<MyType, Key, IndexKey, Blocks> walker(*this, block_num, sum);
        Base::walkRange(start, end, walker);
    }

    void sum(Int start, Int end, Accumulator& accum) const
    {
        SumsWalker<MyType, Key, IndexKey, Accumulator> walker(*this, accum);
        Base::walkRange(start, end, walker);
    }

    Int findSumPositionFw(Int block_num, Int start, Key key, Accumulator& acc) const
    {
        FindSumPositionFwFn<MyType, Key, IndexKey, Blocks> walker(*this, block_num, key);
        return Base::walkFw(start, walker);
    }

    Int findSumPositionBw(Int block_num, Int start, Key key, Accumulator& acc) const
    {
        FindSumPositionBwFn<MyType, Key, IndexKey, Blocks> walker(*this, block_num, key);
        return Base::walkBw(start, walker);
    }


    Int findSumPositionFw(Int block_num, Int start, Key key, IndexKey& acc) const
    {
        FindSumPositionFwFn<MyType, Key, IndexKey, Blocks> walker(*this, block_num, key);

        Int position = Base::walkFw(start, walker);

        acc += walker.sum();

        return position;
    }

    Int findSumPositionBw(Int block_num, Int start, Key key, IndexKey& acc) const
    {
        FindSumPositionBwFn<MyType, Key, IndexKey, Blocks> walker(*this, block_num, key);

        Int position = Base::walkBw(start, walker);

        acc += walker.sum();

        return position;
    }

    Int findSumPositionBwLT(Int block_num, Int start, Key key, IndexKey& acc) const
    {
        FindSumPositionBwLTFn<MyType, Key, IndexKey, Blocks> walker(*this, block_num, key);

        Int position = Base::walkBw(start, walker);

        acc += walker.sum();

        return position;
    }

};

}


#endif
