
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

//#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/datatype_buffer/packed_datatype_buffer.hpp>

#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <memoria/core/tools/i7_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/exint_codec.hpp>

namespace memoria {
namespace tests {

template <typename T> struct TypeTag {};

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
    using TreeSO    = typename PackedTreeT::SparseObject;

    using TreePtr   = PkdStructSPtr<Tree>;

    typedef typename Tree::ViewType                                             Value;
    typedef typename Tree::ViewType                                             IndexValue;
    typedef typename TreeSO::Values                                             Values;

    static constexpr size_t Blocks = Tree::Columns;

    int64_t size_{4096};
    size_t iterations_ {10};

    std::tuple<> ext_data_{};

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

    TreeSO get_so(TreePtr ptr) {
        return TreeSO(&ext_data_, ptr.get());
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

    std::vector<Values> fillRandom(TreeSO& tree, size_t size, size_t max_value = 300, size_t min = 1)
    {
        std::vector<Values> vals(size);
        for (auto& v: vals)
        {
            for (size_t b = 0; b < Blocks; b++) {
                v[b] = from_number(TypeTag<Value>(), getRandom(max_value) + min);
            }
        }

        tree.insert_from_fn(0, size, [&](size_t block, size_t idx) noexcept {
            return vals[idx][block];
        });

        truncate(vals, size);

        assert_equals(size, tree.size());

        for (size_t b = 0; b < Blocks; b++)
        {
            for (size_t idx = 0; idx < tree.size(); idx++) {
                auto view = tree.access(b, idx);
                assert_equals(view, vals[idx][b]);
            }
        }

        return vals;
    }


    void fillVector(TreeSO& tree, const std::vector<Values>& vals)
    {
        tree.insert_from_fn(0, vals.size(), [&](size_t block, size_t idx) noexcept {
            return vals[idx][block];
        });
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

    void assertEqual(const TreeSO& tree, const std::vector<Values>& vals)
    {
        assert_equals((size_t)vals.size(), tree.size());

        for (size_t c = 0; c < tree.size(); c++)
        {
            Values v;
            for (size_t b = 0; b < Blocks; b++)
            {
                v[b] = tree.access(b, c);
            }

            assert_equals(vals[c], v, "Index: {}", c);
        }
    }

    void assertEqual(const TreeSO& tree1, const TreeSO& tree2)
    {
        AssertEQ(MA_SRC, tree1.size(), tree2.size());

        for (size_t c = 0; c < tree1.size(); c++)
        {
            Values v1, v2;
            for (size_t b = 0; b < Blocks; b++)
            {
                v1[b] = tree1.value(b, c);
                v2[b] = tree2.value(b, c);
            }

            assert_equals(v1, v2, "Index: {}", c);
        }
    }

    void assertIndexCorrect(const char* src, const TreeSO& tree)
    {
        try {
            tree.check();
        }
        catch (Exception& e)
        {
            out() << "Tree structure check failed" << std::endl;
            throw e;
        }
    }

    void assertEmpty(const TreeSO& tree)
    {
        assert_equals(0, tree.size());
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
