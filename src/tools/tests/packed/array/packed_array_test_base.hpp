
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_ARRAY_TEST_BASE_HPP_
#define MEMORIA_TESTS_PACKED_ARRAY_TEST_BASE_HPP_

#include "../../tests_inc.hpp"

#include <memoria/core/packed/array/packed_vle_dense_array.hpp>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename TreeType>
class PackedArrayTestBase: public TestTask {
protected:

    typedef TreeType                                                     		Tree;

    typedef typename Tree::Value                                                Value;
    typedef typename Tree::Values                                               Values;

    static const Int Blocks                                                     = 1;

public:

    PackedArrayTestBase(StringRef name): TestTask(name)
    {}

    Tree* createEmptyTree(Int block_size = 1024*1024*64)
    {
        void* block = malloc(block_size);

        PackedAllocator* allocator = T2T<PackedAllocator*>(block);

        allocator->init(block_size, 1);
        allocator->setTopLevelAllocator();

        Tree* tree = allocator->template allocateEmpty<Tree>(0);

        return tree;
    }

    Tree* createTree(Int tree_capacity, Int free_space = 0)
    {
        Int tree_block_size = Tree::block_size(tree_capacity);

        Int allocator_size  = PackedAllocator::block_size(tree_block_size + free_space, 1);

        void* block = malloc(allocator_size);

        PackedAllocator* allocator = T2T<PackedAllocator*>(block);

        allocator->init(tree_block_size, 1);
        allocator->setTopLevelAllocator();

        Tree* tree = allocator->allocate<Tree>(0, tree_block_size);

        return tree;
    }

    void truncate(vector<Values>& v, Int size) {
        Int delta = v.size() - size;

        for (Int idx = 0; idx < delta; idx++)
        {
            v.erase(v.end() - 1);
        }
    }

    vector<Values> fillRandom(Tree* tree, Int size, Int max_value = 300)
    {
        vector<Values> vals(size);

        for (auto& v: vals) {
        	for (Int b = 0; b < Blocks; b++)
        	{
        		v[b] = getRandom(max_value);
        	}
        }

        tree->insert(0, size, [&](Int block, Int idx) {
            return vals[idx][block];
        });

        return vals;
    }

    vector<Values> fillSolid(Tree* tree, Int size, const Values& values)
    {
        vector<Values> vals(size);

        for (auto& v: vals) {
        	v = values;
        }

        tree->insert(0, [&](Int block, Int idx) -> bool {
            return values[block];
        });

        return vals;
    }

    vector<Values> fillSolid(Tree* tree, Int size, Int value)
    {
        vector<Values> vals(size);

        for (auto& v: vals) {
        	for (Int b = 0; b < Blocks; b++)
        	{
        		v[b] = value;
        	}
        }

        tree->insert(0, [&](Int block, Int idx) -> bool {
        	return value;
        });

        return vals;
    }

    void fillVector(Tree* tree, const vector<Values>& vals)
    {
        tree->insert(0, vals.size(), [&](Int block, Int idx) {
            return vals[idx][block];
        });
    }

    Values createRandom(Int max = 100)
    {
        Values values;

        for (Int c = 0; c < Blocks; c++) {
            values[c] = getRandom(max);
        }

        return values;
    }

    vector<Values> createRandomValuesVector(Int size, Int max_value = 100)
    {
        vector<Values> vals(size);

        for (Int c = 0; c < size; c++)
        {
            for (Int b = 0; b < Blocks; b++)
            {
                vals[c][b] = getRandom(max_value);
            }
        }

        return vals;
    }

    void assertEqual(const Tree* tree, const vector<Values>& vals)
    {
        AssertEQ(MA_SRC, tree->size(), (Int)vals.size());

        for (Int c = 0; c < tree->size(); c++)
        {
            Values v;
            for (Int b = 0; b < Blocks; b++)
            {
                v[b] = tree->value(b, c);
            }

            AssertEQ(MA_SRC, vals[c], v);
        }
    }

    void assertEqual(const Tree* tree1, const Tree* tree2)
    {
        AssertEQ(MA_SRC, tree1->size(), tree2->size());

        for (Int c = 0; c < tree1->size(); c++)
        {
            Values v1, v2;
            for (Int b = 0; b < Blocks; b++)
            {
                v1[b] = tree1->value(b, c);
                v2[b] = tree2->value(b, c);
            }

            AssertEQ(MA_SRC, v1, v2);
        }
    }

    void assertIndexCorrect(const char* src, const Tree* tree)
    {
        try {
            tree->check();
        }
        catch (Exception& e) {
            out()<<"Tree structure check failed"<<std::endl;
            tree->dump(out());
            throw e;
        }
    }

    void assertEmpty(const Tree* tree)
    {
        Int empty_size = Tree::empty_size();

        AssertEQ(MA_SRC, tree->size(), 0);
        AssertEQ(MA_SRC, tree->data_size(), 0);
        AssertEQ(MA_SRC, tree->block_size(), empty_size);
//        AssertEQ(MA_SRC, tree->index_size(), 0);
    }
};

}


#endif

