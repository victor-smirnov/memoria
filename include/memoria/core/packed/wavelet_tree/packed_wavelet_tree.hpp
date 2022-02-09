
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
    static const size_t BitsPerLabel       = 8;
};

template <typename Types>
class PackedWaveletTree: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
    typedef PackedWaveletTree<Types>                                            MyType;

    typedef typename Types::Allocator                                           Allocator;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

    struct CardinalTreeTypes {
        static const size_t BitsPerLabel   = Types::BitsPerLabel;
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

    size_t size() const {
        return msequence()->subseq_size(0);
    }

    void insert(size_t idx, uint64_t value)
    {
        insert(ctree()->tree()->root(), idx, value, 3);
    }

    void remove(size_t idx) {
        removeValue(idx, ctree()->tree()->root(), 3);
    }


    uint64_t value(size_t idx) const
    {
        uint64_t value = 0;

        buildValue(idx, ctree()->tree()->root(), value, 3);

        return value;
    }


    size_t rank(size_t idx, uint64_t symbol) const
    {
        return buildRank(ctree()->tree()->root(), idx, symbol, 3);
    }


    size_t select(size_t rank, uint64_t symbol) const
    {
        return select(ctree()->tree()->root(), rank, symbol, 3) - 1;
    }


    static size_t empty_size()
    {
        size_t ctree_block_size    = CardinalTree::empty_size();
        size_t multiseq_block_size = MultiSequence::empty_size();

        size_t block_size = Base::block_size(ctree_block_size + multiseq_block_size, 2);

        return block_size;
    }

    VoidResult init()
    {
        size_t block_size = empty_size();

        MEMORIA_TRY_VOID(Base::init(block_size, 2));
        MEMORIA_TRY_VOID(Base::template allocate_empty<CardinalTree>(CTREE));
        MEMORIA_TRY_VOID(Base::template allocate_empty<MultiSequence>(MULTISEQ));

        return VoidResult::of();
    }

    static size_t block_size(size_t client_area)
    {
        return Base::block_size(client_area, 2);
    }

    void dump(ostream& out = cout, bool dump_index = true) const
    {
        out<<"Cardinal Tree:"<<endl;
        ctree()->dump(out, dump_index);

        out<<"MultiSequence:"<<endl;
        msequence()->dump(out, true, dump_index);
    }

private:

    void insert(const PackedLoudsNode& node, size_t idx, uint64_t value, size_t level)
    {
        size_t label = (value >> (level * 8)) & 0xFF;

        size_t seq_num = node.rank1() - 1;

        msequence()->insertSymbol(seq_num, idx, label);
        size_t rank = msequence()->rank(seq_num, idx + 1, label);

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


    void buildValue(size_t idx, const PackedLoudsNode& node, uint64_t& value, size_t level) const
    {
        size_t     seq_num = node.rank1() - 1;
        uint64_t label   = msequence()->symbol(seq_num, idx);

        size_t     rank    = msequence()->rank(seq_num, idx + 1, label);

        value |= label << (level * 8);

        if (level > 0)
        {
            PackedLoudsNode child = ctree()->find_child(node, label);

            buildValue(rank - 1, child, value, level - 1);
        }
    }

    void removeValue(size_t idx, const PackedLoudsNode& node, size_t level)
    {
        size_t     seq_num = node.rank1() - 1;
        uint64_t label   = msequence()->symbol(seq_num, idx);
        size_t     rank    = msequence()->rank(seq_num, idx + 1, label);

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

    size_t select(const PackedLoudsNode& node, size_t rank, uint64_t symbol, size_t level) const
    {
        if (level >= 0)
        {
            size_t label = (symbol >> (level * 8)) & 0xFFull;

            PackedLoudsNode child = ctree()->find_child(node, label);

            size_t rnk = select(child, rank, symbol, level - 1);

            size_t seq_num = node.rank1() - 1;
            return msequence()->select(seq_num, rnk, label).local_pos() + 1;
        }
        else {
            return rank;
        }
    }


    size_t buildRank(const PackedLoudsNode& node, size_t idx, uint64_t symbol, size_t level) const
    {
        size_t seq_num = node.rank1() - 1;
        size_t label   = (symbol >> (level * 8)) & 0xFFull;
        size_t rank    = msequence()->rank(seq_num, idx + 1, label);

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
