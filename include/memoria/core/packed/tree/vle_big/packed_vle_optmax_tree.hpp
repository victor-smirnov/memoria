
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

#include <memoria/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/core/packed/tree/vle_big/packed_vle_optmax_tree_so.hpp>

#include <memoria/core/tools/optional.hpp>

namespace memoria {

template <typename Types> class PkdVMOTree;


template <typename ValueT, template <typename> class CodecT = ValueCodec, int32_t kBranchingFactor = 1024>
using PkdVMOTreeT = PkdVMOTree<PkdVBMTreeTypes<ValueT, CodecT, kBranchingFactor>>;


template <typename Types>
class PkdVMOTree: public PackedAllocator {

    using Base      = PackedAllocator;
    using MyType    = PkdVMOTree<Types>;

public:

    static constexpr uint32_t VERSION = 1;
    static constexpr int32_t Blocks = Types::Blocks;

    using Tree      = PkdVBMTree<Types>;
    using Bitmap    = PkdFSSeq<typename PkdFSSeqTF<1>::Type>;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                typename Tree::FieldsList,
                typename Bitmap::FieldsList,
                ConstValue<uint32_t, VERSION>
    >;



    enum {BITMAP, TREE, STRUCTS_NUM__};


    using IndexValue = typename Tree::Value;

    using TreeValue  = typename Tree::Value;
    using TreeValues = typename Tree::Values;

    using Value      = typename Tree::Value;
    using Values     = core::StaticVector<Value, Blocks>;
    using SizesT     = typename Tree::SizesT;

    using ExtData = typename DataTypeTraits<typename Tree::DataType>::ExtData;
    using SparseObject = PackedVLEOptMaxTreeSO<ExtData, MyType>;

    using Base::block_size;

    Bitmap* bitmap() {
        return this->template get<Bitmap>(BITMAP);
    }

    const Bitmap* bitmap() const {
        return this->template get<Bitmap>(BITMAP);
    }

    Tree* tree() {
        return this->template get<Tree>(TREE);
    }

    const Tree* tree() const {
        return this->template get<Tree>(TREE);
    }

    // FIXME: Why it doesn't take PackedAllocator's space into account?
    static int32_t empty_size()
    {
        int32_t parent_size = PackedAllocator::empty_size(STRUCTS_NUM__);
        return parent_size + Bitmap::empty_size() + Tree::empty_size();
    }


    static int32_t block_size(int32_t capacity)
    {
        int32_t parent_size = PackedAllocator::empty_size(STRUCTS_NUM__);
        return parent_size + Bitmap::packed_block_size(capacity) + Tree::empty_size();
    }

    int32_t block_size(const MyType* other) const
    {
        return MyType::block_size(size() + other->size());
    }

    VoidResult init_bs(int32_t block_size) noexcept {
        return init();
    }

    static int32_t elements_for(int32_t block_size) {
        return -1;
    }

    VoidResult init(int32_t capacity = 0) noexcept
    {
        MEMORIA_TRY_VOID(Base::init(block_size(capacity), STRUCTS_NUM__));

        int32_t bitmap_block_size = Bitmap::packed_block_size(capacity);

        MEMORIA_TRY_VOID(bitmap, allocateSpace<Bitmap>(BITMAP, bitmap_block_size));
        MEMORIA_TRY_VOID(bitmap->init(bitmap_block_size));

        MEMORIA_TRY(tree, allocateSpace<Tree>(TREE, Tree::empty_size()));

        return tree->init();
    }

    VoidResult init(const SizesT& sizes) noexcept
    {
        return MyType::init(sizes[0]);
    }



    int32_t size() const
    {
        return bitmap()->size();
    }


    template <typename T>
    void max(core::StaticVector<T, Blocks>& accum) const
    {
        const Tree* tree = this->tree();

        int32_t size = tree->size();

        if (size > 0)
        {
            for (int32_t block = 0; block < Blocks; block++)
            {
                accum[block] = tree->value(block, size - 1);
            }
        }
        else {
            for (int32_t block = 0; block < Blocks; block++)
            {
                accum[block] = Value();
            }
        }
    }



    const Value value(int32_t block, int32_t idx) const
    {
        const Bitmap* bitmap = this->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            int32_t tree_idx = this->tree_idx(bitmap, idx);
            return tree()->value(block, tree_idx);
        }
        else {
            return Value();
        }
    }

    Values get_values(int32_t idx) const
    {
        Values v;

        auto bitmap = this->bitmap();

        if (bitmap->symbol(idx) == 1)
        {
            auto tree = this->tree();
            int32_t tree_idx = this->tree_idx(idx);

            OptionalAssignmentHelper(v, tree->get_values(tree_idx));
        }

        return v;
    }


    template <typename T>
    VoidResult setValues(int32_t idx, const core::StaticVector<T, Blocks>& values) noexcept
    {
        Bitmap* bitmap  = this->bitmap();
        Tree* tree      = this->tree();

        if (values[0].is_set())
        {
            TreeValues tree_values  = this->tree_values(values);
            int32_t tree_idx        = this->tree_idx(idx);

            if (bitmap->symbol(idx))
            {
                MEMORIA_TRY_VOID(tree->setValues(tree_idx, tree_values));
            }
            else {
                MEMORIA_TRY_VOID(tree->insert(tree_idx, tree_values));
                bitmap->symbol(idx) = 1;
                MEMORIA_TRY_VOID(bitmap->reindex());
            }
        }
        else {
            int32_t tree_idx = this->tree_idx(idx);

            if (bitmap->symbol(idx))
            {
                MEMORIA_TRY_VOID(tree->remove(tree_idx, tree_idx + 1));
                bitmap->symbol(idx) = 0;
                MEMORIA_TRY_VOID(bitmap->reindex());
            }
            else {
                // Do nothing
            }
        }

        return VoidResult::of();
    }


    template <typename T>
    auto findGTForward(int32_t block, const T& val) const
    {
        auto result = tree()->find_gt(block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findGTForward(int32_t block, const Optional<T>& val) const
    {
        auto result = tree()->find_gt(block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findGEForward(int32_t block, const T& val) const
    {
        auto result = tree()->find_ge(block, val.value());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findForward(SearchType search_type, int32_t block, const T& val) const
    {
        auto result = tree()->findForward(search_type, block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findForward(SearchType search_type, int32_t block, const Optional<T>& val) const
    {
        auto result = tree()->findForward(search_type, block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }


    template <typename T>
    auto findBackward(SearchType search_type, int32_t block, const T& val) const
    {
        auto result = tree()->findBackward(search_type, block, val);

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }

    template <typename T>
    auto findBackward(SearchType search_type, int32_t block, const Optional<T>& val) const
    {
        auto result = tree()->findBackward(search_type, block, val.get());

        result.set_idx(global_idx(result.local_pos()));

        return result;
    }



    VoidResult reindex()
    {
        MEMORIA_TRY_VOID(bitmap()->reindex());
        return tree()->reindex();
    }

    void check() const
    {
        bitmap()->check();
        tree()->check();
    }


    VoidResult splitTo(MyType* other, int32_t idx) noexcept
    {
        Bitmap* bitmap = this->bitmap();

        int32_t tree_idx = this->tree_idx(bitmap, idx);

        MEMORIA_TRY_VOID(bitmap->splitTo(other->bitmap(), idx));

        MEMORIA_TRY_VOID(tree()->splitTo(other->tree(), tree_idx));

        return reindex();
    }

    VoidResult mergeWith(MyType* other) noexcept
    {
        MEMORIA_TRY_VOID(bitmap()->mergeWith(other->bitmap()));

        return tree()->mergeWith(other->tree());
    }

    VoidResult removeSpace(int32_t start, int32_t end) noexcept
    {
        return remove(start, end);
    }

    VoidResult remove(int32_t start, int32_t end) noexcept
    {
        Bitmap* bitmap = this->bitmap();

        int32_t tree_start = tree_idx(bitmap, start);
        int32_t tree_end = tree_idx(bitmap, end);

        MEMORIA_TRY_VOID(bitmap->remove(start, end));

        return tree()->remove(tree_start, tree_end);
    }

    template <typename T>
    VoidResult insert(int32_t idx, const core::StaticVector<T, Blocks>& values) noexcept
    {
        Bitmap* bitmap  = this->bitmap();

        if (values[0].is_set())
        {
            MEMORIA_TRY_VOID(bitmap->insert(idx, 1));

            TreeValues tree_values  = this->tree_values(values);
            int32_t tree_idx        = this->tree_idx(bitmap, idx);

            Tree* tree = this->tree();
            return tree->insert(tree_idx, tree_values);
        }
        else {
            return bitmap->insert(idx, 0);
        }
    }


//    VoidResult insert(int32_t idx, int32_t size, std::function<const Values& (int32_t)> provider, bool reindex = true)
//    {
//        auto bitmap = this->bitmap();
//        int32_t bitidx = 0;
//        int32_t setted = 0;

//        bitmap->insert(idx, size, [&]() {
//            auto v = provider(bitidx++);
//            setted += v[0].is_set();
//            return v[0].is_set();
//        });

//        auto tree = this->tree();
//        int32_t tree_idx = this->tree_idx(bitmap, idx);

//        int tidx = 0;

//        TreeValues tv;

//        if(isFail(tree->insert(tree_idx, setted, [&, this](int32_t) -> const auto& {
//            while(true)
//            {
//                if (tidx < size)
//                {
//                    const auto& v = provider(tidx++);

//                    if (v[0].is_set())
//                    {
//                        tv = this->tree_values(v);



//                        return tv;
//                    }
//                }
//                else {
//                    MMA_THROW(Exception()) << WhatInfo(format_u8("Position {} exceeds {}", tidx, size));
//                }
//            }
//        }))) {
//            return VoidResult::FAIL;
//        };

//        return VoidResult::OK;
//    }



    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startStruct();
        handler->startGroup("VMO_TREE");
        
        MEMORIA_TRY_VOID(bitmap()->generateDataEvents(handler));
        MEMORIA_TRY_VOID(tree()->generateDataEvents(handler));

        handler->endGroup();
        handler->endStruct();

        return VoidResult::of();
    }

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::serialize(buf));

        MEMORIA_TRY_VOID(bitmap()->serialize(buf));
        return tree()->serialize(buf);
    }

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        MEMORIA_TRY_VOID(bitmap()->deserialize(buf));
        return tree()->deserialize(buf);
    }

    void dump() const {
        bitmap()->dump();
        tree()->dump();
    }

protected:

    template <typename T>
    core::StaticVector<TreeValue, Blocks> tree_values(const core::StaticVector<Optional<T>, Blocks>& values)
    {
        core::StaticVector<TreeValue, Blocks> tv;

        for (int32_t b = 0;  b < Blocks; b++)
        {
            tv[b] = values[b].get();
        }

        return tv;
    }

    int32_t tree_idx(int32_t global_idx) const
    {
        return tree_idx(bitmap(), global_idx);
    }

    int32_t tree_idx(const Bitmap* bitmap, int32_t global_idx) const
    {
        int32_t rank = bitmap->rank(global_idx, 1);
        return rank;
    }


    int32_t global_idx(int32_t tree_idx) const
    {
        return global_idx(bitmap(), tree_idx);
    }

    int32_t global_idx(const Bitmap* bitmap, int32_t tree_idx) const
    {
        auto result = bitmap->selectFw(1, tree_idx + 1);
        return result.local_pos();
    }
};


}
