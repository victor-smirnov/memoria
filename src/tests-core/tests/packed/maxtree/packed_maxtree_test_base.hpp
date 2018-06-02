
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/v1/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/v1/core/tools/i7_codec.hpp>
#include <memoria/v1/core/tools/elias_codec.hpp>
#include <memoria/v1/core/tools/exint_codec.hpp>

#include <memoria/v1/core/bignum/bigint.hpp>

#include <memoria/v1/reactor/reactor.hpp>

namespace memoria {
namespace v1 {
namespace tests {


template <typename PackedTreeT>
class PackedMaxTreeTestBase: public TestState {
    using Base   = TestState;
    using MyType = PackedMaxTreeTestBase;

protected:

    static constexpr int32_t MEMBUF_SIZE = 1024*1024*64;


    using Tree      = PackedTreeT;
    using TreePtr   = PkdStructSPtr<Tree>;

    typedef typename Tree::Value                                                Value;
    typedef typename Tree::IndexValue                                           IndexValue;
    typedef typename Tree::Values                                               Values;

    static constexpr int32_t Blocks = Tree::Blocks;

    int32_t size_{};

public:

    using Base::getRandom;
    using Base::out;

    MMA1_STATE_FILEDS(size_);

    TreePtr createEmptyTree(int32_t block_size = MEMBUF_SIZE)
    {
        return MakeSharedPackedStructByBlock<Tree>(block_size);
    }

    TreePtr createTree(int32_t tree_capacity, int32_t free_space = 0)
    {
        int32_t tree_block_size = Tree::block_size(tree_capacity);
        return MakeSharedPackedStructByBlock<Tree>(tree_block_size + free_space);
    }

    void truncate(std::vector<Values>& v, int32_t size) {
        int32_t delta = v.size() - size;

        for (int32_t idx = 0; idx < delta; idx++)
        {
            v.erase(v.end() - 1);
        }
    }

    std::vector<Values> fillRandom(TreePtr& tree, int32_t size, int32_t max_value = 300, int32_t min = 1)
    {
        Values accum;

        std::vector<Values> vals(size);
        for (auto& v: vals)
        {
            for (int32_t b = 0; b < Blocks; b++) {
                v[b] = accum[b] + getRandom(max_value) + min;
                accum[b] = v[b];
            }
        }

        OOM_THROW_IF_FAILED(tree->_insert(0, size, [&](int32_t block, int32_t idx) {
            return vals[idx][block];
        }), MMA1_SRC);

        truncate(vals, size);

        assert_equals(size, tree->size());

        dumpVector(this->out(), vals);


        for (int32_t b = 0; b < Blocks; b++)
        {
            int32_t idx = 0;
            tree->read(b, 0, tree->size(), make_fn_with_next([&](int32_t block, auto v){
                assert_equals(v, vals[idx][block]);
            }, [&]{idx++;}));
        }

        return vals;
    }



    void fillVector(TreePtr& tree, const std::vector<Values>& vals)
    {
        OOM_THROW_IF_FAILED(tree->_insert(0, vals.size(), [&](int32_t block, int32_t idx) {
            return vals[idx][block];
        }), MMA1_SRC);
    }

    Values createRandom(int32_t max = 100)
    {
        Values values;

        for (int32_t c = 0; c < Blocks; c++) {
            values[c] = getRandom(max);
        }

        return values;
    }

    std::vector<Values> createRandomValuesVector(int32_t size, int32_t max_value = 300)
    {
        std::vector<Values> vals(size);

        for (int32_t c = 0; c < size; c++)
        {
            for (int32_t b = 0; b < Blocks; b++)
            {
                vals[c][b] = getRandom(max_value);
            }
        }

        return vals;
    }

    void assertEqual(const TreePtr& tree, const std::vector<Values>& vals)
    {
        assert_equals(tree->size(), (int32_t)vals.size());

        for (int32_t c = 0; c < tree->size(); c++)
        {
            Values v;
            for (int32_t b = 0; b < Blocks; b++)
            {
                v[b] = tree->value(b, c);
            }

            assert_equals(vals[c], v, u"Index: {}", c);
        }
    }

    void assertEqual(const TreePtr& tree1, const TreePtr& tree2)
    {
        assert_equals(tree1->size(), tree2->size());

        for (int32_t c = 0; c < tree1->size(); c++)
        {
            Values v1, v2;
            for (int32_t b = 0; b < Blocks; b++)
            {
                v1[b] = tree1->value(b, c);
                v2[b] = tree2->value(b, c);
            }

            assert_equals(v1, v2, u"Index: {}", c);
        }
    }

    void assertIndexCorrect(const char* src, const TreePtr& tree)
    {
        try {
            tree->check();
        }
        catch (Exception& e) {
            out() << "Tree structure check failed" << std::endl;
            tree->dump(out());
            throw e;
        }
    }

    void assertEmpty(const TreePtr& tree)
    {
        assert_equals(0, tree->size());
    }

    template <typename T>
    void dump(const std::vector<T>& v, std::ostream& out)
    {
        for (int32_t c = 0; c < v.size(); c++) {
            out << c << ": " << v[c] << std::endl;
        }
    }
};

}}}
