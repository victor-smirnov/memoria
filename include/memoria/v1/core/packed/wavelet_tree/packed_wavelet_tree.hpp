
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memoria/v1/core/packed/louds/packed_louds_cardinal_tree.hpp>
#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/v1/core/packed/sseq/packed_multisequence.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

namespace memoria {
namespace v1 {

template <typename Allocator_ = PackedAllocator>
struct PackedWaveletTreeTypes {
    typedef Allocator_                  Allocator;
    static const Int BitsPerLabel       = 8;
};

template <typename Types>
class PackedWaveletTree: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
    typedef PackedWaveletTree<Types>                                            MyType;

    typedef typename Types::Allocator                                           Allocator;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

    struct CardinalTreeTypes {
        static const Int BitsPerLabel   = Types::BitsPerLabel;
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

    Int size() const {
        return msequence()->subseq_size(0);
    }

    void insert(Int idx, UBigInt value)
    {
        insert(ctree()->tree()->root(), idx, value, 3);
    }

    void remove(Int idx) {
        removeValue(idx, ctree()->tree()->root(), 3);
    }


    UBigInt value(Int idx) const
    {
        UBigInt value = 0;

        buildValue(idx, ctree()->tree()->root(), value, 3);

        return value;
    }


    Int rank(Int idx, UBigInt symbol) const
    {
        return buildRank(ctree()->tree()->root(), idx, symbol, 3);
    }


    Int select(Int rank, UBigInt symbol) const
    {
        return select(ctree()->tree()->root(), rank, symbol, 3) - 1;
    }


    static Int empty_size()
    {
        Int ctree_block_size    = CardinalTree::empty_size();
        Int multiseq_block_size = MultiSequence::empty_size();

        Int block_size = Base::block_size(ctree_block_size + multiseq_block_size, 2);

        return block_size;
    }

    void init()
    {
        Int block_size = empty_size();

        Base::init(block_size, 2);

        Base::template allocateEmpty<CardinalTree>(CTREE);
        Base::template allocateEmpty<MultiSequence>(MULTISEQ);
    }

    static Int block_size(Int client_area)
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

    void insert(const PackedLoudsNode& node, Int idx, UBigInt value, Int level)
    {
        Int label = (value >> (level * 8)) & 0xFF;

        Int seq_num = node.rank1() - 1;

        msequence()->insertSymbol(seq_num, idx, label);
        Int rank = msequence()->rank(seq_num, idx + 1, label);

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


    void buildValue(Int idx, const PackedLoudsNode& node, UBigInt& value, Int level) const
    {
        Int     seq_num = node.rank1() - 1;
        UBigInt label   = msequence()->symbol(seq_num, idx);

        Int     rank    = msequence()->rank(seq_num, idx + 1, label);

        value |= label << (level * 8);

        if (level > 0)
        {
            PackedLoudsNode child = ctree()->find_child(node, label);

            buildValue(rank - 1, child, value, level - 1);
        }
    }

    void removeValue(Int idx, const PackedLoudsNode& node, Int level)
    {
        Int     seq_num = node.rank1() - 1;
        UBigInt label   = msequence()->symbol(seq_num, idx);
        Int     rank    = msequence()->rank(seq_num, idx + 1, label);

        if (level > 0)
        {
            PackedLoudsNode child = ctree()->find_child(node, label);

            removeValue(rank - 1, child, level - 1);
        }

        auto seq = this->msequence();

        seq->removeSymbol(seq_num, idx);

        if (seq->length(seq_num) == 0 && node.idx() > 0)
        {
            seq->remove(seq_num);
            ctree()->removeLeaf(node);
        }
    }

    Int select(const PackedLoudsNode& node, Int rank, UBigInt symbol, Int level) const
    {
        if (level >= 0)
        {
            Int label = (symbol >> (level * 8)) & 0xFFull;

            PackedLoudsNode child = ctree()->find_child(node, label);

            Int rnk = select(child, rank, symbol, level - 1);

            Int seq_num = node.rank1() - 1;
            return msequence()->select(seq_num, rnk, label).idx() + 1;
        }
        else {
            return rank;
        }
    }


    Int buildRank(const PackedLoudsNode& node, Int idx, UBigInt symbol, Int level) const
    {
        Int seq_num = node.rank1() - 1;
        Int label   = (symbol >> (level * 8)) & 0xFFull;
        Int rank    = msequence()->rank(seq_num, idx + 1, label);

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


}}