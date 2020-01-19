
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/core/packed/array/packed_fse_array.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/core/integer/integer.hpp>

#include "../tree/packed_tree_test_base.hpp"

namespace memoria {
namespace tests {

template <typename TreeType>
class PackedArrayTestBase: public TestState {
protected:
    using Base = TestState;

    typedef TreeType                                                            Array;
    typedef PkdStructSPtr<Array>                                                ArrayPtr;

    typedef typename Array::Value                                               Value;
    typedef typename Array::Values                                              Values;

    static const int32_t Blocks                                                 = Array::Blocks;

    int32_t iterations_ = 10;
    int64_t size_;

public:

    MMA1_STATE_FILEDS(size_, iterations_);

    ArrayPtr createEmptyArray(int32_t block_size = 1024*1024*64)
    {
        return MakeSharedPackedStructByBlock<Array>(block_size);
    }

    ArrayPtr createArray(int32_t tree_capacity, int32_t free_space = 0)
    {
        int32_t tree_block_size = Array::block_size(tree_capacity);

        return MakeSharedPackedStructByBlock<Array>(tree_block_size + free_space);
    }

    void truncate(std::vector<Values>& v, int32_t size) {
        int32_t delta = v.size() - size;

        for (int32_t idx = 0; idx < delta; idx++)
        {
            v.erase(v.end() - 1);
        }
    }

    std::vector<Values> fillRandom(ArrayPtr& tree, int32_t size, int32_t max_value = 300)
    {
        std::vector<Values> vals(size);

        for (auto& v: vals) {
            for (int32_t b = 0; b < Blocks; b++)
            {
                v[b] = getRandom(max_value);
            }
        }

        OOM_THROW_IF_FAILED(tree->insert(0, size, [&](int32_t block, int32_t idx) {
            return vals[idx][block];
        }), MMA1_SRC);

        return vals;
    }

    std::vector<Values> fillSolid(ArrayPtr& tree, int32_t size, const Values& values)
    {
        std::vector<Values> vals(size);

        for (auto& v: vals) {
            v = values;
        }

        tree->insert(0, [&](int32_t block, int32_t idx) -> bool {
            return values[block];
        });

        return vals;
    }

    std::vector<Values> fillSolid(ArrayPtr& tree, int32_t size, int32_t value)
    {
        std::vector<Values> vals(size);

        for (auto& v: vals) {
            for (int32_t b = 0; b < Blocks; b++)
            {
                v[b] = value;
            }
        }

        tree->insert(0, [&](int32_t block, int32_t idx) -> bool {
            return value;
        });

        return vals;
    }

    void fillVector(ArrayPtr& tree, const std::vector<Values>& vals)
    {
        OOM_THROW_IF_FAILED(tree->insert(0, vals.size(), [&](int32_t block, int32_t idx) {
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

    std::vector<Values> createRandomValuesVector(int32_t size, int32_t max_value = 100)
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

    void assertEqual(const ArrayPtr& tree, const std::vector<Values>& vals)
    {
        assert_equals(tree->size(), (int32_t)vals.size());

        for (int32_t c = 0; c < tree->size(); c++)
        {
            Values v;
            for (int32_t b = 0; b < Blocks; b++)
            {
                v[b] = tree->value(b, c);
            }

            assert_equals(vals[c], v, "{}", c);
        }
    }

    void assertEqual(const ArrayPtr& tree1, const ArrayPtr& tree2)
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

            AssertEQ(MA_SRC, v1, v2);
        }
    }

    void assertIndexCorrect(const char* src, const ArrayPtr& tree)
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

    void assertEmpty(const ArrayPtr& tree)
    {
        assert_equals(0, tree->size());
        assert_equals(0, tree->data_size());
    }
};

}}
