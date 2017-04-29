
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

#include <memoria/v1/tools/tests_inc.hpp>

#include <memoria/v1/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>

namespace memoria {
namespace v1 {

using namespace std;

template <typename TreeType>
class PackedArrayTestBase: public TestTask {
protected:

    typedef TreeType                                                            Array;
    typedef PkdStructSPtr<Array>                                                ArrayPtr;

    typedef typename Array::Value                                               Value;
    typedef typename Array::Values                                              Values;

    static const Int Blocks                                                     = Array::Blocks;

public:

    PackedArrayTestBase(StringRef name): TestTask(name)
    {}

    ArrayPtr createEmptyArray(Int block_size = 1024*1024*64)
    {
        return MakeSharedPackedStructByBlock<Array>(block_size);
    }

    ArrayPtr createArray(Int tree_capacity, Int free_space = 0)
    {
        Int tree_block_size = Array::block_size(tree_capacity);

        return MakeSharedPackedStructByBlock<Array>(tree_block_size + free_space);
    }

    void truncate(vector<Values>& v, Int size) {
        Int delta = v.size() - size;

        for (Int idx = 0; idx < delta; idx++)
        {
            v.erase(v.end() - 1);
        }
    }

    vector<Values> fillRandom(ArrayPtr& tree, Int size, Int max_value = 300)
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

    vector<Values> fillSolid(ArrayPtr& tree, Int size, const Values& values)
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

    vector<Values> fillSolid(ArrayPtr& tree, Int size, Int value)
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

    void fillVector(ArrayPtr& tree, const vector<Values>& vals)
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

    void assertEqual(const ArrayPtr& tree, const vector<Values>& vals)
    {
        AssertEQ(MA_SRC, tree->size(), (Int)vals.size());

        for (Int c = 0; c < tree->size(); c++)
        {
            Values v;
            for (Int b = 0; b < Blocks; b++)
            {
                v[b] = tree->value(b, c);
            }

            AssertEQ(MA_SRC, vals[c], v, SBuf()<<c);
        }
    }

    void assertEqual(const ArrayPtr& tree1, const ArrayPtr& tree2)
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

    void assertIndexCorrect(const char* src, const ArrayPtr& tree)
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

    void assertEmpty(const ArrayPtr& tree)
    {
        AssertEQ(MA_SRC, tree->size(), 0);
        AssertEQ(MA_SRC, tree->data_size(), 0);
    }
};

}}