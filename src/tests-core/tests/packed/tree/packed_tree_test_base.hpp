
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

#include <memoria/core/integer/integer.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/exint_codec.hpp>

namespace memoria {
namespace tests {


template <typename R, typename T>
R from_number(TypeTag<R> tag, T value);

template <typename T>
UUID from_number(TypeTag<UUID>, T value)
{
    return UUID(value, value);
}

template <size_t BitLength, typename T>
UnsignedAccumulator<BitLength> from_number(TypeTag<UnsignedAccumulator<BitLength>>, T value)
{
    return UnsignedAccumulator<BitLength>(value);
}


template <typename R, typename T>
R from_number(TypeTag<R>, T value)
{
    return value;
}

template <typename PackedTreeT>
class PackedTreeTestBase: public TestState {
    using Base = TestState;
protected:

    static constexpr size_t MEMBUF_SIZE = 1024*1024*8;

    using Tree      = PackedTreeT;
    using TreePtr   = PkdStructSPtr<Tree>;

    typedef typename Tree::Value                                                Value;
    typedef typename Tree::IndexValue                                           IndexValue;
    typedef typename Tree::DataValues                                           Values;

    static constexpr size_t Blocks = Tree::Blocks;

    int64_t size_{4096};
    size_t iterations_ {10};

public:

    using Base::getRandom;
    using Base::out;

    MMA_STATE_FILEDS(size_, iterations_);

    template <typename T>
    IndexValue sum(const std::vector<T>& tree, size_t block, size_t start, size_t end)
    {
        IndexValue sum = 0;

        for (size_t c = start; c < end; c++)
        {
            sum += tree[c][block];
        }

        return sum;
    }


    TreePtr createEmptyTree(size_t block_size = MEMBUF_SIZE)
    {
        return MakeSharedPackedStructByBlock<Tree>(block_size);
    }

    TreePtr createTree(size_t tree_capacity, size_t free_space = 0)
    {
        size_t tree_block_size = Tree::block_size(tree_capacity);
        return MakeSharedPackedStructByBlock<Tree>(tree_block_size + free_space);
    }

    void truncate(std::vector<Values>& v, size_t size) {
        size_t delta = v.size() - size;

        for (size_t idx = 0; idx < delta; idx++)
        {
            v.erase(v.end() - 1);
        }
    }

    std::vector<Values> fillRandom(TreePtr& tree, size_t size, size_t max_value = 300, size_t min = 1)
    {
        std::vector<Values> vals(size);
        for (auto& v: vals)
        {
            for (size_t b = 0; b < Blocks; b++) {
                v[b] = from_number(TypeTag<Value>(), getRandom(max_value) + min);
            }
        }

        tree->insert_entries(0, size, [&](size_t block, size_t idx) noexcept {
            return vals[idx][block];
        }).get_or_throw();

        truncate(vals, size);

        assert_equals(size, tree->size());

        for (size_t b = 0; b < Blocks; b++)
        {
            size_t idx = 0;
            tree->read(b, 0, tree->size(), make_fn_with_next([&](size_t block, auto v){
                assert_equals(v, vals[idx][block]);
            }, [&]{idx++;}));
        }

        return vals;
    }

    std::vector<Values> fillSolid(TreePtr& tree, const Values& values)
    {
        std::vector<Values> vals;

        size_t size = tree->insert(0, [&](Values& v) -> bool {
            v = values;
            vals.push_back(values);

            return true;
        });

        truncate(vals, size);

        return vals;
    }

    std::vector<Values> fillSolid(Tree* tree, size_t value)
    {
        std::vector<Values> vals;

        size_t size = tree->insert(0, [&](Values& v) -> bool {
            for (size_t b = 0; b < Blocks; b++) {
                v[b] = value;
            }

            vals.push_back(v);

            return true;
        });

        truncate(vals, size);

        return vals;
    }

    void fillVector(TreePtr& tree, const std::vector<Values>& vals)
    {
        tree->insert_entries(0, vals.size(), [&](size_t block, size_t idx) noexcept {
            return vals[idx][block];
        }).get_or_throw();
    }

    Values createRandom(size_t max = 100)
    {
        Values values;

        for (size_t c = 0; c < Blocks; c++) {
            values[c] = from_number(TypeTag<Value>(), getRandom(max));
        }

        return values;
    }

    std::vector<Values> createRandomValuesVector(size_t size, size_t max_value = 300)
    {
        std::vector<Values> vals(size);

        for (size_t c = 0; c < size; c++)
        {
            for (size_t b = 0; b < Blocks; b++)
            {
                vals[c][b] = from_number(TypeTag<Value>(), getRandom(max_value));
            }
        }

        return vals;
    }

    void assertEqual(const TreePtr& tree, const std::vector<Values>& vals)
    {
        assert_equals(tree->size(), (size_t)vals.size());

        for (size_t c = 0; c < tree->size(); c++)
        {
            Values v;
            for (size_t b = 0; b < Blocks; b++)
            {
                v[b] = tree->value(b, c);
            }

            assert_equals(vals[c], v, "Index: {}", c);
        }
    }

    void assertEqual(const TreePtr& tree1, const TreePtr& tree2)
    {
        AssertEQ(MA_SRC, tree1->size(), tree2->size());

        for (size_t c = 0; c < tree1->size(); c++)
        {
            Values v1, v2;
            for (size_t b = 0; b < Blocks; b++)
            {
                v1[b] = tree1->value(b, c);
                v2[b] = tree2->value(b, c);
            }

            assert_equals(v1, v2, "Index: {}", c);
        }
    }

    void assertIndexCorrect(const char* src, const TreePtr& tree)
    {
        try {
            tree->check();
        }
        catch (Exception& e)
        {
            out() << "Tree structure check failed" << std::endl;
            //tree->dump(out());
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
        for (size_t c = 0; c < v.size(); c++) {
            out << c << ": " << v[c] << std::endl;
        }
    }
};

}}
