
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

#include <memoria/core/packed/louds/packed_louds_tree.hpp>
#include <memoria/core/packed/array/packed_fse_bitmap.hpp>

#include <functional>

namespace memoria {

template <typename Types>
class PackedLoudsCardinalTree: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
    typedef PackedLoudsCardinalTree<Types>                                      MyType;

public:
    static const size_t SafetyGap                                                  = 8;
    static const size_t BitsPerLabel                                               = Types::BitsPerLabel;

    typedef LoudsTreeTypes<>                                                    TreeTypes;
    typedef PackedLoudsTree<TreeTypes>                                          LoudsTree;

    typedef PackedFSEBitmapTypes<BitsPerLabel>                                  LabelArrayTypes;
    typedef PackedFSEBitmap<LabelArrayTypes>                                    LabelArray;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

    enum {
        TREE, LABELS
    };

public:
    PackedLoudsCardinalTree() {}

    LoudsTree* tree() {
        return Base::template get<LoudsTree>(TREE);
    }

    const LoudsTree* tree() const {
        return Base::template get<LoudsTree>(TREE);
    }

    LabelArray* labels() {
        return Base::template get<LabelArray>(LABELS);
    }

    const LabelArray* labels() const {
        return Base::template get<LabelArray>(LABELS);
    }

    size_t label(const PackedLoudsNode& node) const
    {
        return labels(node.rank1());
    }

    PackedLoudsNode insertNode(const PackedLoudsNode& at, size_t label)
    {
        PackedLoudsNode node = tree()->insertNode(at);
        MEMORIA_V1_ASSERT_TRUE(node);
        MEMORIA_V1_ASSERT_TRUE(labels()->insert(node.rank1() - 1, label));

        return node;
    }

    void removeLeaf(const PackedLoudsNode& node)
    {
        size_t idx = node.rank1() - 1;
        labels()->removeSpace(idx, idx + 1);
        tree()->removeLeaf(node);
    }

    VoidResult init() noexcept
    {
        size_t block_size = empty_size();

        MEMORIA_TRY_VOID(Base::init(block_size, 2));

        MEMORIA_TRY_VOID(Base::template allocate_empty<LoudsTree>(TREE));

        MEMORIA_TRY_VOID(Base::template allocate_empty<LabelArray>(LABELS));

        return VoidResult::of();
    }

    PackedLoudsNode find_child(const PackedLoudsNode& node, size_t label) const
    {
        const LoudsTree* louds = tree();
        PackedLoudsNodeSet children = louds->children(node);

        const LabelArray* labels = this->labels();

        for (size_t c = 0; c < children.length(); c++)
        {
            size_t node_label = labels->value(children.rank1() + c - 1);

            if (node_label >= label)
            {
                return PackedLoudsNode(children.local_pos() + c, children.rank1() + c);
            }
        }

        return PackedLoudsNode(children.local_pos() + children.length(), children.rank1() + children.length() - 1);
    }


    void insert_path(uint64_t path, size_t size, function<void (const PackedLoudsNode&, size_t label, size_t level)> fn)
    {
        LoudsTree* louds = tree();

        PackedLoudsNode node = louds->root();

        size_t level = 0;

        while (!louds->isLeaf(node))
        {
            size_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            node = find_child(node, label);

            size_t lbl = labels()->value(node.rank1() - 1);

            if (!louds->isLeaf(node) && lbl == label)
            {
                level++;
            }
            else {
                int a = 0; a++;
                break;
            }
        }

        bool first = true;

        if (level < size)
        {
            while (level < size)
            {
                louds = this->tree();

                louds->insert(node.local_pos(), 1, 2 - first);
                louds->reindex();

                node = louds->node(node.local_pos()); // refresh

                size_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

                labels()->insert(node.rank1() - 1, label);

                louds->reindex();

                MEMORIA_ASSERT(louds, ==, this->tree());

                fn(node, label, level);

                node = louds->first_child(node);

                level++;
                first = false;
            }

            tree()->insert(node.local_pos(), 0, 1);
            tree()->reindex();
        }
    }

    bool query_path(uint64_t path, size_t size, function<void (const PackedLoudsNode&, size_t label, size_t level)> fn) const
    {
        LoudsTree* louds = tree();

        PackedLoudsNode node = louds->root();

        size_t level = 0;

        while (!louds->isLeaf(node))
        {
            size_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            PackedLoudsNode child = find_child(node, label);

            MEMORIA_ASSERT(child.is_empty(), !=, true);

            level++;
        }

        return level < size;
    }

    enum class Status {
        LEAF, NOT_FOUND, OK, FINISH
    };

    Status remove_path(const PackedLoudsNode& node, uint64_t path, size_t size, size_t level)
    {
        LoudsTree* louds = tree();
        if (!louds->isLeaf(node))
        {
            size_t label = GetBits(&path, level * BitsPerLabel, BitsPerLabel);

            PackedLoudsNode child = find_child(node, label);

            if (child.is_empty())
            {
                return Status::NOT_FOUND;
            }
            else {
                Status status = remove_path(child, path, size, level + 1);

                if (status == Status::OK)
                {
                    bool alone = louds->isAlone(node);

                    if (node.local_pos() > 0)
                    {
                        removeLeaf(node);
                    }

                    if (alone)
                    {
                        return Status::OK;
                    }
                    else {
                        return Status::FINISH;
                    }
                }
                else {
                    return status;
                }
            }
        }
        else {
            return Status::OK;
        }
    }

    bool remove_path(uint64_t path, size_t size)
    {
        PackedLoudsNode node = tree()->root();
        return remove_path(node, path, size, 0) != Status::NOT_FOUND;
    }

    void prepare()
    {
        tree()->insert(0, 1, 3);
        tree()->reindex();

        labels()->insertSpace(0, 1);
        labels()->value(0) = 0;
    }


    static size_t block_size(size_t client_area)
    {
        return Base::block_size(client_area, 2);
    }

    static size_t empty_size()
    {
        size_t tree_block_size     = PackedAllocator::round_up_bytes_to_alignment_blocks(LoudsTree::empty_size());
        size_t labels_block_size   = PackedAllocator::round_up_bytes_to_alignment_blocks(LabelArray::empty_size());

        size_t client_area = tree_block_size + labels_block_size;

        size_t bs = MyType::block_size(client_area);
        return bs;
    }

    void dump(ostream& out = cout, bool dump_index = true) const
    {
//      if (dump_index)
//      {
//          Base::dump(out);
//      }

        out<<"Louds Tree: "<<endl;
        tree()->dump(out, dump_index);
        out<<endl;

        out<<"Cardinal Labels: "<<endl;
        labels()->dump(out);
        out<<endl;
    }
};

}
