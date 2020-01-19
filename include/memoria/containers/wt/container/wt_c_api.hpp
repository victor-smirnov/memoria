
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

#include <memoria/core/container/container.hpp>

#include <memoria/containers/wt/wt_names.hpp>

#include <functional>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(wt::CtrApiName)
public:
    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Seq                                                  Seq;

    typedef typename Tree::Iterator                                             TreeIterator;
    typedef typename Seq::Iterator                                              SeqIterator;
    typedef typename Tree::Types::CtrSizeT                                      CtrSizeT;

    void prepare()
    {
        auto& self = this->self();
        auto& tree = self.tree();

        auto iter = tree->seek(0);

        iter->insertNode(std::make_tuple(0, 0));
        iter->next();
        iter->insertZero();
    }

    CtrSizeT size()
    {
        auto& self = this->self();
        auto root = self.tree()->seek(0);

        if (!root->isEof())
        {
            return std::get<1>(root->labels());
        }
        else {
            return 0;
        }
    }

    void insert(int32_t idx, uint64_t value)
    {
        auto& self = this->self();
        auto& tree = self.tree();

        auto root = tree->seek(0);

        insert(*root.get(), idx, value, 3);
    }

    void remove(int32_t idx)
    {
        auto& self = this->self();
        auto& tree = self.tree();

        auto root = tree->seek(0);

        removeValue(idx, *root.get(), 3);
    }



    uint64_t value(int32_t idx)
    {
        uint64_t value = 0;

        auto& self = this->self();
        auto& tree = self.tree();

        auto root = tree->seek(0);

        buildValue(idx, *root.get(), value, 3);

        return value;
    }


    CtrSizeT rank(CtrSizeT idx, uint64_t symbol)
    {
        auto& self = this->self();
        auto& tree = self.tree();

        auto root = tree->seek(0);

        return buildRank(*root.get(), idx, symbol, 3);
    }


    CtrSizeT select(CtrSizeT rank, uint64_t symbol)
    {
        auto& self = this->self();
        auto& tree = self.tree();

        auto root = tree->seek(0);

        return select(*root.get(), rank, symbol, 3) - 1;
    }

private:

    void insert(TreeIterator& node, int32_t idx, uint64_t value, int32_t level)
    {
        auto& self = this->self();
        auto& tree = self.tree();
        auto& seq  = self.seq();

        int32_t label = (value >> (level * 8)) & 0xFF;

        int64_t seq_base = node.template sumLabel<1>();

        seq->insert_symbol(seq_base + idx, label);
        node.template addLabel<1>(1);

        int64_t rank = seq->rank(seq_base, idx + 1, label);

        if (level > 0)
        {
            auto child = self.findChild(node, label);

            if (rank == 1)
            {
                tree->newNodeAt(*child.get(), std::make_tuple(label, 0));
            }

            insert(*child.get(), rank - 1, value, level - 1);
        }
    }


    void buildValue(int32_t idx, TreeIterator& node, uint64_t& value, int32_t level)
    {
        auto& self = this->self();
        auto& seq  = self.seq();

        CtrSizeT seq_base   = node.template sumLabel<1>();
        uint64_t label       = seq->seek(seq_base + idx)->symbol();
        CtrSizeT  rank      = seq->rank(seq_base, idx + 1, label);

        value |= label << (level * 8);

        if (level > 0)
        {
            auto child = self.findChild(node, label);
            buildValue(rank - 1, *child.get(), value, level - 1);
        }
    }


    void removeValue(int32_t idx, TreeIterator& node, int32_t level)
    {
        auto& self = this->self();
        auto& tree = self.tree();
        auto& seq  = self.seq();

        CtrSizeT node_pos   = node.pos();

        CtrSizeT seq_base   = node.template sumLabel<1>();
        uint64_t label       = seq->seek(seq_base + idx)->symbol();
        CtrSizeT  rank      = seq->rank(seq_base, idx + 1, label);

        if (level > 0)
        {
            auto child = self.findChild(node, label);
            removeValue(rank - 1, *child.get(), level - 1);
        }

        node = *tree->seek(node_pos).get();

        seq_base = node.template sumLabel<1>();
        seq->seek(seq_base + idx)->remove();
        node.template addLabel<1>(-1);

        CtrSizeT seq_length = std::get<1>(node.labels());

        if (seq_length == 0)
        {
            if (node.pos() > 0)
            {
                tree->removeLeaf(node);
            }
        }
    }


    CtrSizeT select(TreeIterator& node, int32_t rank, uint64_t symbol, int32_t level)
    {
        auto& self = this->self();
        auto& seq  = self.seq();

        if (level >= 0)
        {
            int32_t label = (symbol >> (level * 8)) & 0xFFull;

            auto child = self.findChild(node, label);

            CtrSizeT rnk = select(*child.get(), rank, symbol, level - 1);

            CtrSizeT seq_base = node.template sumLabel<1>();

            int64_t pos = seq->select(seq_base, rnk, label)->pos() + 1;

            return pos - seq_base;
        }
        else {
            return rank;
        }
    }


    CtrSizeT buildRank(TreeIterator& node, CtrSizeT idx, uint64_t symbol, int32_t level)
    {
        int32_t label  = (symbol >> (level * 8)) & 0xFFull;

        auto& self = this->self();
        auto& seq  = self.seq();

        CtrSizeT seq_base = node.template sumLabel<1>();

        CtrSizeT rank = seq->rank(seq_base, idx + 1, label);

        if (level > 0)
        {
            auto child = self.findChild(node, label);
            return buildRank(*child.get(), rank - 1, symbol, level - 1);
        }
        else {
            return rank;
        }
    }


MEMORIA_V1_CONTAINER_PART_END

}
