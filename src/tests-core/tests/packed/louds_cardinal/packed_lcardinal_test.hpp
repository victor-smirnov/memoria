
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

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_lcardinal_test_base.hpp"

#include <memory>

namespace memoria {

class PackedLoudsCardinalTest: public PackedLoudsCardinalTreeTestBase {

    typedef PackedLoudsCardinalTreeTestBase                                     Base;
    typedef PackedLoudsCardinalTest                                             MyType;



    using CardinalTree = typename Base::Tree;
    using CardinalTreePtr = typename Base::TreePtr;

    typedef typename CardinalTree::LoudsTree                                    LoudsTree;
    typedef typename CardinalTree::LabelArray                                   LabelArray;


public:

    PackedLoudsCardinalTest(): Base("Create")
    {
        MEMORIA_ADD_TEST(testCreateCardinalTree);
        MEMORIA_ADD_TEST(testRemoveCardinalTree);
    }

    virtual ~PackedLoudsCardinalTest() noexcept {}

    CardinalTreePtr createCardinalTree(int32_t block_size = 64*1024)
    {
        return MakeSharedPackedStructByBlock<CardinalTree>(block_size);
    }

    uint64_t buildPath(PackedLoudsNode node, int32_t level, const CardinalTreePtr& ctree)
    {
        const LoudsTree* tree       = ctree->tree();
        const LabelArray* labels    = ctree->labels();

        uint64_t path = 0;

        for (int32_t l = level - 1; l >= 0; l--)
        {
            uint64_t label = labels->value(node.rank1() - 1);

            path |= label << (8 * l);

            node = tree->parent(node);
        }

        return path;
    }

    void checkTreeContent(const CardinalTreePtr& tree, set<uint64_t>& paths)
    {
        traverseTreePaths(tree, [this, tree, &paths](const PackedLoudsNode& node, int32_t level) {
            AssertEQ(MA_SRC, level, 4);
            uint64_t path = buildPath(node, level, tree);
            AssertTrue(MA_SRC, paths.find(path) != paths.end());
        });
    }


    void testCreateCardinalTree()
    {
        auto tree = createCardinalTree();

        tree->prepare();

        auto fn = [](const PackedLoudsNode& node, int32_t label, int32_t level){};

        set<uint64_t> paths;

        for (int32_t c = 0; c < 1000; c++)
        {
            uint32_t path = getRandom();

            out()<<c<<" "<<hex<<path<<dec<<endl;

            paths.insert(path);

            tree->insert_path(path, 4, fn);

            checkTreeContent(tree, paths);
        }

        out()<<"Free space in the tree: "<<tree->free_space()<<endl;
    }


    void testRemoveCardinalTree()
    {
        auto tree = createCardinalTree();

        tree->prepare();

        auto fn = [](const PackedLoudsNode& node, int32_t label, int32_t level){};

        set<uint64_t> paths;

        for (int32_t c = 0; c < 100; c++)
        {
            uint32_t path = getRandom();

            out()<<c<<" "<<hex<<path<<dec<<endl;

            paths.insert(path);

            tree->insert_path(path, 4, fn);

            checkTreeContent(tree, paths);
        }



        while (paths.size() > 0)
        {
            int32_t idx = getRandom(paths.size());
            uint64_t path;

            for (auto p: paths)
            {
                if (idx-- == 0)
                {
                    path = p;
                    break;
                }
            }

            out()<<"Remove: "<<hex<<path<<dec<<" "<<paths.size()<<endl;

            bool result = tree->remove_path(path, 4);
            AssertTrue(MA_SRC, result);

            paths.erase(path);

            checkTreeContent(tree, paths);
        }

        tree->dump(out());
    }
};


}
