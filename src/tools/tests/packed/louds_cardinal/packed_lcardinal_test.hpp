
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_LOUDS_CARDINAL_HPP_
#define MEMORIA_TESTS_PACKED_LOUDS_CARDINAL_HPP_

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "packed_lcardinal_test_base.hpp"

#include <memory>

namespace memoria {

using namespace std;

class PackedLoudsCardinalTest: public PackedLoudsCardinalTreeTestBase {

    typedef PackedLoudsCardinalTreeTestBase                                     Base;
    typedef PackedLoudsCardinalTest                                             MyType;

    typedef typename Base::Tree                                                 CardinalTree;

    typedef typename CardinalTree::LoudsTree                                    LoudsTree;
    typedef typename CardinalTree::LabelArray                                   LabelArray;


public:

    PackedLoudsCardinalTest(): Base("Create")
    {
        MEMORIA_ADD_TEST(testCreateCardinalTree);
        MEMORIA_ADD_TEST(testRemoveCardinalTree);
    }

    virtual ~PackedLoudsCardinalTest() throw() {}

    CardinalTree* createCardinalTree(Int block_size = 64*1024)
    {
        PackedAllocator* alloc = T2T<PackedAllocator*>(malloc(block_size));

        alloc->init(block_size, 1);
        alloc->setTopLevelAllocator();

        CardinalTree* tree = alloc->template allocateEmpty<CardinalTree>(0);

        return tree;
    }

    UBigInt buildPath(PackedLoudsNode node, Int level, const CardinalTree* ctree)
    {
        const LoudsTree* tree       = ctree->tree();
        const LabelArray* labels    = ctree->labels();

        UBigInt path = 0;

        for (Int l = level - 1; l >= 0; l--)
        {
            UBigInt label = labels->value(node.rank1() - 1);

            path |= label << (8 * l);

            node = tree->parent(node);
        }

        return path;
    }

    void checkTreeContent(const CardinalTree* tree, set<UBigInt>& paths)
    {
        traverseTreePaths(tree, [this, tree, &paths](const PackedLoudsNode& node, Int level) {
            AssertEQ(MA_SRC, level, 4);
            UBigInt path = buildPath(node, level, tree);
            AssertTrue(MA_SRC, paths.find(path) != paths.end());
        });
    }


    void testCreateCardinalTree()
    {
        CardinalTree* tree = createCardinalTree();
        PARemover remover(tree);

        tree->prepare();

        auto fn = [](const PackedLoudsNode& node, Int label, Int level){};

        set<UBigInt> paths;

        for (Int c = 0; c < 1000; c++)
        {
            UInt path = getRandom();

            out()<<c<<" "<<hex<<path<<dec<<endl;

            paths.insert(path);

            tree->insert_path(path, 4, fn);

            checkTreeContent(tree, paths);
        }

        out()<<"Free space in the tree: "<<tree->free_space()<<endl;
    }


    void testRemoveCardinalTree()
    {
        CardinalTree* tree = createCardinalTree();
        PARemover remover(tree);

        tree->prepare();

        auto fn = [](const PackedLoudsNode& node, Int label, Int level){};

        set<UBigInt> paths;

        for (Int c = 0; c < 100; c++)
        {
            UInt path = getRandom();

            out()<<c<<" "<<hex<<path<<dec<<endl;

            paths.insert(path);

            tree->insert_path(path, 4, fn);

            checkTreeContent(tree, paths);
        }

        while (paths.size() > 0)
        {
            Int idx = getRandom(paths.size());
            UBigInt path;

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


#endif
