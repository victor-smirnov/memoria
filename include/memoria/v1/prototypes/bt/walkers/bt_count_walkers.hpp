
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

#include <memoria/v1/prototypes/bt/walkers/bt_find_walkers.hpp>

#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {
namespace bt {


/**********************************************************************/

template <
    typename Types,
    typename MyType
>
class CountForwardWalkerBase: public FindForwardWalkerBase<Types, MyType> {
protected:
    using Base       = FindForwardWalkerBase<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

    using typename Base::LeafPath;

    using Base::sum_;



public:

    using Base::leaf_index;
    using Base::treeNode;

    CountForwardWalkerBase():
        Base(-1, 0, SearchType::GE)
    {}

    struct FindBranchFn {
        MyType& walker_;

        FindBranchFn(MyType& walker): walker_(walker) {}

        template <Int ListIdx, typename StreamType, typename... Args>
        StreamOpResult stream(const StreamType* stream, bool root, Int index, Int start, Args&&... args)
        {
            StreamOpResult result = walker_.template find_non_leaf<ListIdx>(stream, root, index, start, std::forward<Args>(args)...);

            // TODO: should we also forward args... to this call?
            walker_.template postProcessBranchStream<ListIdx>(stream, start, result.idx());

            return result;
        }
    };

    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}

    template <typename NodeTypes>
    StreamOpResult treeNode(const bt::BranchNode<NodeTypes>* node, WalkDirection direction, Int start)
    {
        auto& self = this->self();

        Int index = node->template translateLeafIndexToBranchIndex<LeafPath>(self.leaf_index());

        using BranchPath         = typename bt::BranchNode<NodeTypes>::template BuildBranchPath<LeafPath>;
        using BranchSizePath = IntList<ListHead<LeafPath>::Value>;

        auto sizes_substream = node->template substream<BranchSizePath>();

        auto result = node->template processStream<BranchPath>(FindBranchFn(self), node->is_root(), index, start, sizes_substream);

        self.postProcessBranchNode(node, direction, start, result);

        return result;
    }



    template <Int StreamIdx, typename Tree, typename SizesSubstream>
    StreamOpResult find_non_leaf(const Tree* tree, bool root, Int index, Int start, const SizesSubstream* sizes_substream)
    {
        auto size = tree->size();

        if (start < size)
        {
            MEMORIA_V1_ASSERT(tree->size(), ==, sizes_substream->size());

            auto tree_iter  = tree->iterator(index, start);
            auto sizes_iter = sizes_substream->iterator(0, start);

            for (Int c = start; c < size; c++)
            {
                sizes_iter.next();
                tree_iter.next();

                if (tree_iter.value() != sizes_iter.value())
                {
                    return StreamOpResult(c, start, false, false);
                }
                else {
                    sum_ += sizes_iter.value();
                }
            }

            return StreamOpResult(size, start, true, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }


    template <Int StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq* seq, Int start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq);

        auto count = seq->count(start);

        sum_ += count.count();

        this->set_leaf_index(count.symbol());

        Int end = start + count.count();

        return StreamOpResult(end, start, end >= seq->size());
    }
};




template <
    typename Types
>
class CountForwardWalker: public CountForwardWalkerBase<Types, CountForwardWalker<Types>> {

    using Base = CountForwardWalkerBase<Types, CountForwardWalker<Types>>;

public:
    CountForwardWalker(): Base() {}
};







}
}}
