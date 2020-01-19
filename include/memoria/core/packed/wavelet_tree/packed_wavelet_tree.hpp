
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/types.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/core/packed/louds/packed_louds_cardinal_tree.hpp>
#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/core/packed/sseq/packed_multisequence.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria {

template <typename Allocator_ = PackedAllocator>
struct PackedWaveletTreeTypes {
    typedef Allocator_                  Allocator;
    static const int32_t BitsPerLabel       = 8;
};

template <typename Types>
class PackedWaveletTree: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
    typedef PackedWaveletTree<Types>                                            MyType;

    typedef typename Types::Allocator                                           Allocator;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

    struct CardinalTreeTypes {
        static const int32_t BitsPerLabel   = Types::BitsPerLabel;
    };

    typedef PackedCxMultiSequenceTypes<
            Types::BitsPerLabel
    >                                                                           MultiSequenceTypes;

public:
    typedef PackedLoudsCardinalTree<CardinalTreeTypes>                          CardinalTree;
    typedef PackedCxMultiSequence<MultiSequenceTypes>                           MultiSequence;

    enum {
        CTREE, MULTISEQ
    };


public:

    PackedWaveletTree() {}


    CardinalTree* ctree()
    {
        return Base::template get<CardinalTree>(CTREE);
    }

    const CardinalTree* ctree() const
    {
        return Base::get<CardinalTree>(CTREE);
    }

    MultiSequence* msequence()
    {
        return Base::template get<MultiSequence>(MULTISEQ);
    }

    const MultiSequence* msequence() const
    {
        return Base::template get<MultiSequence>(MULTISEQ);
    }

    void prepare()
    {
        ctree()->prepare();
        msequence()->insertSubsequence(0);
    }

    int32_t size() const {
        return msequence()->subseq_size(0);
    }

    void insert(int32_t idx, uint64_t value)
    {
        insert(ctree()->tree()->root(), idx, value, 3);
    }

    void remove(int32_t idx) {
        removeValue(idx, ctree()->tree()->root(), 3);
    }


    uint64_t value(int32_t idx) const
    {
        uint64_t value = 0;

        buildValue(idx, ctree()->tree()->root(), value, 3);

        return value;
    }


    int32_t rank(int32_t idx, uint64_t symbol) const
    {
        return buildRank(ctree()->tree()->root(), idx, symbol, 3);
    }


    int32_t select(int32_t rank, uint64_t symbol) const
    {
        return select(ctree()->tree()->root(), rank, symbol, 3) - 1;
    }


    static int32_t empty_size()
    {
        int32_t ctree_block_size    = CardinalTree::empty_size();
        int32_t multiseq_block_size = MultiSequence::empty_size();

        int32_t block_size = Base::block_size(ctree_block_size + multiseq_block_size, 2);

        return block_size;
    }

    OpStatus init()
    {
        int32_t block_size = empty_size();

        if(isFail(Base::init(block_size, 2))) {
            return OpStatus::FAIL;
        }

        if(isFail(Base::template allocateEmpty<CardinalTree>(CTREE))) {
            return OpStatus::FAIL;
        }
        if(isFail(Base::template allocateEmpty<MultiSequence>(MULTISEQ))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }

    static int32_t block_size(int32_t client_area)
    {
        return Base::block_size(client_area, 2);
    }

    void dump(ostream& out = cout, bool dump_index = true) const
    {
//      if (dump_index)
//      {
//          Base::dump(out);
//      }

        out<<"Cardinal Tree:"<<endl;
        ctree()->dump(out, dump_index);

        out<<"MultiSequence:"<<endl;
        msequence()->dump(out, true, dump_index);
    }

private:

    void insert(const PackedLoudsNode& node, int32_t idx, uint64_t value, int32_t level)
    {
        int32_t label = (value >> (level * 8)) & 0xFF;

        int32_t seq_num = node.rank1() - 1;

        msequence()->insertSymbol(seq_num, idx, label);
        int32_t rank = msequence()->rank(seq_num, idx + 1, label);

        if (level > 0)
        {
            PackedLoudsNode child = ctree()->find_child(node, label);

            if (rank == 1)
            {
                child = ctree()->insertNode(child, label);
                msequence()->insertSubsequence(child.rank1() - 1);
            }

            insert(child, rank - 1, value, level - 1);
        }
    }


    void buildValue(int32_t idx, const PackedLoudsNode& node, uint64_t& value, int32_t level) const
    {
        int32_t     seq_num = node.rank1() - 1;
        uint64_t label   = msequence()->symbol(seq_num, idx);

        int32_t     rank    = msequence()->rank(seq_num, idx + 1, label);

        value |= label << (level * 8);

        if (level > 0)
        {
            PackedLoudsNode child = ctree()->find_child(node, label);

            buildValue(rank - 1, child, value, level - 1);
        }
    }

    void removeValue(int32_t idx, const PackedLoudsNode& node, int32_t level)
    {
        int32_t     seq_num = node.rank1() - 1;
        uint64_t label   = msequence()->symbol(seq_num, idx);
        int32_t     rank    = msequence()->rank(seq_num, idx + 1, label);

        if (level > 0)
        {
            PackedLoudsNode child = ctree()->find_child(node, label);

            removeValue(rank - 1, child, level - 1);
        }

        auto seq = this->msequence();

        seq->removeSymbol(seq_num, idx);

        if (seq->length(seq_num) == 0 && node.local_pos() > 0)
        {
            seq->remove(seq_num);
            ctree()->removeLeaf(node);
        }
    }

    int32_t select(const PackedLoudsNode& node, int32_t rank, uint64_t symbol, int32_t level) const
    {
        if (level >= 0)
        {
            int32_t label = (symbol >> (level * 8)) & 0xFFull;

            PackedLoudsNode child = ctree()->find_child(node, label);

            int32_t rnk = select(child, rank, symbol, level - 1);

            int32_t seq_num = node.rank1() - 1;
            return msequence()->select(seq_num, rnk, label).local_pos() + 1;
        }
        else {
            return rank;
        }
    }


    int32_t buildRank(const PackedLoudsNode& node, int32_t idx, uint64_t symbol, int32_t level) const
    {
        int32_t seq_num = node.rank1() - 1;
        int32_t label   = (symbol >> (level * 8)) & 0xFFull;
        int32_t rank    = msequence()->rank(seq_num, idx + 1, label);

        if (level > 0)
        {
            PackedLoudsNode child = ctree()->find_child(node, label);
            return buildRank(child, rank - 1, symbol, level - 1);
        }
        else {
            return rank;
        }
    }


};

}
