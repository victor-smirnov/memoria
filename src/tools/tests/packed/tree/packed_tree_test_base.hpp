
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_TREE_TEST_BASE_HPP_
#define MEMORIA_TESTS_PACKED_TREE_TEST_BASE_HPP_

#include "../../tests_inc.hpp"

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/exint_codec.hpp>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <typename PackedTreeT>
class PackedTreeTestBase: public TestTask {
	using Base = TestTask;
protected:

	static constexpr Int MEMBUF_SIZE = 1024*1024*64;


    using Tree = PackedTreeT;

    typedef typename Tree::Value                                                Value;
    typedef typename Tree::IndexValue                                           IndexValue;
    typedef typename Tree::Values                                               Values;

    static constexpr Int Blocks = Tree::Blocks;

public:

    using Base::getRandom;

    PackedTreeTestBase(StringRef name): TestTask(name)
    {}

    template <typename T>
    IndexValue sum(const vector<T>& tree, Int block, Int start, Int end)
    {
    	IndexValue sum = 0;

    	for (Int c = start; c < end; c++)
    	{
    		sum += tree[c][block];
    	}

    	return sum;
    }


    Tree* createEmptyTree()
    {
        void* block = malloc(MEMBUF_SIZE);
        PackedAllocator* allocator = T2T<PackedAllocator*>(block);
        allocator->init(MEMBUF_SIZE, 1);
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
        for (auto& v: vals)
        {
        	for (Int b = 0; b < Blocks; b++) {
        		v[b] = getRandom(max_value);
        	}
        }

        tree->_insert(0, size, [&](Int block, Int idx) {
        	return vals[idx][block];
        });

        truncate(vals, size);

        AssertEQ(MA_SRC, size, tree->size());

        for (Int b = 0; b < Blocks; b++)
        {
        	tree->scan(b, 0, tree->size(), [&](Int idx, auto v){
        		AssertEQ(MA_SRC, v, vals[idx][b]);
        	});
        }

        return vals;
    }

    vector<Values> fillSolid(Tree* tree, const Values& values)
    {
        vector<Values> vals;

        Int size = tree->insert(0, [&](Values& v) -> bool {
            v = values;
            vals.push_back(values);

            return true;
        });

        truncate(vals, size);

        return vals;
    }

    vector<Values> fillSolid(Tree* tree, Int value)
    {
        vector<Values> vals;

        Int size = tree->insert(0, [&](Values& v) -> bool {
            for (Int b = 0; b < Blocks; b++) {
                v[b] = value;
            }

            vals.push_back(v);

            return true;
        });

        truncate(vals, size);

        return vals;
    }

    void fillVector(Tree* tree, const vector<Values>& vals)
    {
        tree->_insert(0, vals.size(), [&](Int block, Int idx) {
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

    vector<Values> createRandomValuesVector(Int size, Int max_value = 300)
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

            AssertEQ(MA_SRC, vals[c], v, SBuf()<<"Index: "<<c);
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

            AssertEQ(MA_SRC, v1, v2, SBuf()<<"Index: "<<c);
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
        AssertEQ(MA_SRC, tree->block_size(), empty_size);
    }

    template <typename T>
    void dump(const std::vector<T>& v, std::ostream& out = std::cout)
    {
    	for (Int c = 0; c < v.size(); c++) {
    		out<<c<<": "<<v[c]<<endl;
    	}
    }
};

}


#endif

