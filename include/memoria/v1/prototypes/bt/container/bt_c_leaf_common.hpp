
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::LeafCommonName)
public:
    using Types = typename Base::Types;

protected:
    typedef typename Base::Allocator                                            Allocator;
    
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    using CtrSizeT = typename Types::CtrSizeT;

    static const int32_t Streams = Types::Streams;

public:
    NodeBaseG ctr_split_leaf(NodeBaseG& left_node, const Position& split_at)
    {
        auto& self = this->self();

        return self.ctr_split_node(left_node, [&self, &split_at](NodeBaseG& left, NodeBaseG& right){
            return self.ctr_split_leaf_node(left, right, split_at);
        });
    }

    MEMORIA_V1_DECLARE_NODE_FN_RTN(SplitNodeFn, splitTo, OpStatus);
    OpStatus ctr_split_leaf_node(NodeBaseG& src, NodeBaseG& tgt, const Position& split_at)
    {
        return self().leaf_dispatcher().dispatch(src, tgt, SplitNodeFn(), split_at);
    }

public:










    template <int32_t Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto ctr_apply_substreams_fn(NodeBaseG& leaf, Fn&& fn, Args&&... args)
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t Stream, typename SubstreamsIdxList, typename Fn, typename... Args>
    auto ctr_apply_substreams_fn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t Stream, typename Fn, typename... Args>
    auto ctr_apply_stream_fn(const NodeBaseG& leaf, Fn&& fn, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::StreamNodeFn<Stream>(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <int32_t Stream, typename SubstreamsIdxList, typename... Args>
    auto ctr_read_substreams(const NodeBaseG& leaf, Args&&... args) const
    {
         return self().template ctr_apply_substreams_fn<Stream, SubstreamsIdxList>(leaf, bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }



    template <int32_t Stream, typename... Args>
    auto ctr_read_stream(const NodeBaseG& leaf, Args&&... args) const
    {
         return self().template ctr_apply_stream_fn<Stream>(leaf, bt::GetLeafValuesFn(), std::forward<Args>(args)...);
    }







    struct SumFn {
        template <typename Stream>
        auto stream(const Stream& s, int32_t block, int32_t from, int32_t to)
        {
            return s->sum(block, from, to);
        }

        template <typename Stream>
        auto stream(const Stream& s, int32_t from, int32_t to)
        {
            return s->sums(from, to);
        }
    };



    struct FindFn {
        template <typename Stream, typename... Args>
        auto stream(const Stream& s, Args&&... args)
        {
            return s->findForward(std::forward<Args>(args)...).local_pos();
        }
    };

    template <int32_t Stream, typename SubstreamsIdxList, typename... Args>
    auto ctr_find_forward(const NodeBaseG& leaf, Args&&... args) const
    {
        return self().leaf_dispatcher().dispatch(leaf, bt::SubstreamsSetNodeFn<Stream, SubstreamsIdxList>(), FindFn(), std::forward<Args>(args)...);
    }



    // ============================================ Insert Data ======================================== //


    class LeafList {
        CtrSizeT size_;
        NodeBaseG head_;
        NodeBaseG tail_;
    public:
        LeafList(CtrSizeT size, NodeBaseG head, NodeBaseG tail): size_(size), head_(head), tail_(tail) {}

        CtrSizeT size() const {return size_;}
        const NodeBaseG& head() const {return head_;}
        const NodeBaseG& tail() const {return tail_;}

        NodeBaseG& head() {return head_;}
        NodeBaseG& tail() {return tail_;}
    };



    class InsertDataBlockResult {
        Position inserted_size_;
        bool extra_space_;
    public:
        InsertDataBlockResult(Position size, bool extra_space): inserted_size_(size), extra_space_(extra_space){}

        const Position& inserted_size() const {return inserted_size_;}
        bool has_extra_space() const {return extra_space_;}
    };


    template <typename Provider>
    Position ctr_insert_data_into_leaf(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
        auto& self = this->self();

        self.ctr_update_block_guard(leaf);

        self.ctr_layout_leaf_node(leaf, Position(0));

        if (provider.hasData())
        {
            auto end = provider.fill(leaf, pos);

            return end;
        }

        return pos;
    }


    std::shared_ptr<io::IOVector> create_iovector()
    {
        return std::static_pointer_cast<io::IOVector>(
            std::make_shared<typename Types::LeafNode::template SparseObject<MyType>::IOVectorT>()
        );
    }


    class InsertDataResult {
        NodeBaseG leaf_;
        Position position_;
    public:
        InsertDataResult(NodeBaseG leaf, const Position& position = Position()): leaf_(leaf), position_(position){}

        NodeBaseG& iter_leaf() {return leaf_;}
        const NodeBaseG& iter_leaf() const {return leaf_;}

        const Position& position() const {return position_;}
        Position& position() {return position_;}
    };

    template <typename Provider>
    InsertDataResult ctr_insert_provided_data(NodeBaseG& leaf, const Position& pos, Provider& provider)
    {
        auto& self = this->self();

        auto last_pos = self.ctr_insert_data_into_leaf(leaf, pos, provider);

        if (provider.hasData())
        {
            // has to be defined in subclasses
            if (!self.isAtTheEnd2(leaf, last_pos))
            {
                auto next_leaf = self.ctr_split_leaf(leaf, last_pos);

                self.ctr_insert_data_into_leaf(leaf, last_pos, provider);

                provider.iter_next_leaf(leaf);

                if (provider.hasData())
                {
                    return ctr_insert_data_rest(leaf, next_leaf, provider);
                }
                else {
                    return InsertDataResult(next_leaf, Position());
                }
            }
            else {
                provider.iter_next_leaf(leaf);

                auto next_leaf = self.ctr_get_next_node(leaf);

                if (next_leaf.isSet())
                {
                    return ctr_insert_data_rest(leaf, next_leaf, provider);
                }
                else {
                    return ctr_insert_data_rest(leaf, provider);
                }
            }
        }
        else {
            return InsertDataResult(leaf, last_pos);
        }
    }



    template <typename Provider>
    LeafList ctr_create_leaf_data_list(Provider& provider)
    {
        auto& self = this->self();

        CtrSizeT    total = 0;
        NodeBaseG   head;
        NodeBaseG   current;

        int32_t block_size = self.ctr_get_root_metadata().memory_block_size();

        while (provider.hasData())
        {
            NodeBaseG node = self.ctr_create_node(0, false, true, block_size);

            if (head.isSet())
            {
                current->next_leaf_id() = node->id();
            }
            else {
                head = node;
            }

            self.ctr_insert_data_into_leaf(node, Position(), provider);

            provider.iter_next_leaf(node);

            current = node;

            total++;
        }

        total += provider.orphan_splits();

        return LeafList(total, head, current);
    }




    template <typename Provider>
    InsertDataResult ctr_insert_data_rest(NodeBaseG& leaf, NodeBaseG& next_leaf, Provider& provider)
    {
        auto& self = this->self();

        provider.split_watcher().first = leaf;

        auto leaf_list = self.ctr_create_leaf_data_list(provider);

        if (provider.split_watcher().second.isSet())
        {
            leaf = provider.split_watcher().second;
        }

        int32_t path_parent_idx = leaf->parent_idx() + 1;

        if (leaf_list.size() > 0)
        {
            using LeafListProvider = typename Base::ListLeafProvider;

            LeafListProvider list_provider(self, leaf_list.head(), leaf_list.size());

            NodeBaseG parent = self.ctr_get_node_parent_for_update(leaf);

            self.ctr_insert_subtree(parent, path_parent_idx, list_provider);

            auto& last_leaf = leaf_list.tail();

            auto last_leaf_size = self.ctr_get_leaf_stream_sizes(last_leaf);

            if (self.ctr_merge_leaf_nodes(last_leaf, next_leaf, [](const Position&){}))
            {
                return InsertDataResult(last_leaf, last_leaf_size);
            }
            else {
                return InsertDataResult(next_leaf);
            }
        }
        else {
            return InsertDataResult(next_leaf);
        }
    }


    template <typename Provider>
    InsertDataResult ctr_insert_data_rest(NodeBaseG& leaf, Provider& provider)
    {
        auto& self = this->self();

        if (leaf->is_root())
        {
            self.ctr_create_new_root_block(leaf);
        }

        provider.split_watcher().first = leaf;

        auto leaf_list = self.ctr_create_leaf_data_list(provider);

        if (provider.split_watcher().second.isSet())
        {
            leaf = provider.split_watcher().second;
        }

        int32_t path_parent_idx = leaf->parent_idx() + 1;

        if (leaf_list.size() > 0)
        {
            using LeafListProvider = typename Base::ListLeafProvider;

            LeafListProvider list_provider(self, leaf_list.head(), leaf_list.size());

            NodeBaseG parent = self.ctr_get_node_parent_for_update(leaf);

            self.ctr_insert_subtree(parent, path_parent_idx, list_provider);

            auto& last_leaf = leaf_list.tail();

            auto last_leaf_size = self.ctr_get_leaf_stream_sizes(last_leaf);

            return InsertDataResult(last_leaf, last_leaf_size);
        }
        else {
            auto iter_leaf_size = self.ctr_get_leaf_stream_sizes(leaf);
            return InsertDataResult(leaf, iter_leaf_size);
        }
    }

    template <typename Fn, typename... Args>
    SplitStatus ctr_update_atomic(Iterator& iter, Fn&& fn, Args&&... args)
    {
        auto& self = this->self();

        BlockUpdateMgr mgr(self);

        self.ctr_update_block_guard(iter.iter_leaf());

        mgr.add(iter.iter_leaf());


        self().leaf_dispatcher().dispatch(
            iter.iter_leaf(),
            fn,
            std::forward<Args>(args)...
        );

        if (isOk(fn.status_)) {
            return SplitStatus::NONE;
        }

        mgr.rollback();

        SplitStatus status = iter.split();

        fn.status_ = OpStatus::OK;

        self().leaf_dispatcher().dispatch(
                    iter.iter_leaf(),
                    fn,
                    std::forward<Args>(args)...
                    );

        OOM_THROW_IF_FAILED(fn.status_, MMA1_SRC);

        return status;

    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
