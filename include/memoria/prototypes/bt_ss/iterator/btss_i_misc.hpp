
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

#include <memoria/core/container/macros.hpp>
#include <memoria/core/types.hpp>
#include <memoria/prototypes/bt_ss/btss_names.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/prototypes/bt_ss/btss_input_iovector.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

namespace memoria {

using bt::StreamTag;

MEMORIA_V1_ITERATOR_PART_BEGIN(btss::IteratorMiscName)

    using typename Base::CtrSizeT;
    using typename Base::TreePathT;
    using typename Base::IteratorPtr;
    using typename Base::Container;
    using typename Base::Position;


public:
    int32_t iovector_pos() const noexcept {
        return self().iter_local_pos();
    }

    BoolResult next_leaf() noexcept {
        return self().iter_next_leaf();
    }

    BoolResult next_entry() noexcept
    {
        MEMORIA_TRY(res, self().iter_btss_skip_fw(1));
        return BoolResult::of(res > 0);
    }


    BoolResult operator++() noexcept
    {
        MEMORIA_TRY(res, self().iter_btss_skip_fw(1));
        return BoolResult::of(res > 0);
    }

    BoolResult next() noexcept
    {
        MEMORIA_TRY(res, self().iter_btss_skip_fw(1));
        return BoolResult::of(res > 0);
    }

    BoolResult prev() noexcept
    {
        MEMORIA_TRY(res, self().iter_btss_skip_bw(1));
        return BoolResult::of(res > 0);
    }

    BoolResult operator--() noexcept {
        return prev();
    }

    BoolResult operator++(int) noexcept {
        return next();
    }

    BoolResult operator--(int) noexcept {
        return prev();
    }

    Result<CtrSizeT> operator+=(CtrSizeT size) noexcept {
        return self().iter_btss_skip_fw(size);
    }

    Result<CtrSizeT> operator-=(CtrSizeT size) noexcept {
        return self().iter_btss_skip_bw(size);
    }

    int32_t iter_btss_leaf_size() const noexcept
    {
        return self().iter_leaf_size(0);
    }

//    bool isEof() const {
//        return self().iter_local_pos() >= self().size();
//    }

//    bool isBof() const {
//        return self().iter_local_pos() < 0;
//    }

    Result<CtrSizeT> iter_btss_skip_fw(CtrSizeT amount) noexcept {
        return self().template iter_skip_fw<0>(amount);
    }

    Result<CtrSizeT> iter_btss_skip_bw(CtrSizeT amount) noexcept {
        return self().template iter_skip_bw<0>(amount);
    }

    Result<CtrSizeT> skip(CtrSizeT amount) noexcept {
        return self().template iter_skip<0>(amount);
    }

    struct PosWalker {
        CtrSizeT prefix_{};

        template <typename CtrT, typename NodeT>
        VoidResult treeNode(const BranchNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept
        {
            const auto* stream = node.template substream<IntList<0>>();
            prefix_ += stream->sum(0, start, end);

            return VoidResult::of();
        }


        template <typename CtrT, typename NodeT>
        VoidResult treeNode(const LeafNodeSO<CtrT, NodeT>& node, WalkCmd cmd, int32_t start, int32_t end) noexcept {
            return VoidResult::of();
        }
    };



    Result<CtrSizeT> pos() const noexcept
    {
        using ResultT = Result<CtrSizeT>;
        auto& self = this->self();

        PosWalker walker{};
        MEMORIA_TRY_VOID(self.iter_walk_up_for_refresh(self.path(), 0, self.iter_local_pos(), walker));

        //return ResultT::of(self.iter_local_pos() + self.iter_cache().size_prefix()[0]);
        return ResultT::of(self.iter_local_pos() + walker.prefix_);
    }


    VoidResult iter_remove_current() noexcept
    {
        auto& self  = this->self();
        auto& ctr   = self.ctr();

        ctr.ctr_remove_entry(self);

        if (self.iter_is_end())
        {
            self.iter_skip_fw(0);
        }

        return VoidResult::of();
    }

    Result<CtrSizeT> remove_from(CtrSizeT size) noexcept
    {
        using ResultT = Result<CtrSizeT>;

        auto& self = this->self();

        auto to = self.ctr().clone_iterator(self);

        MEMORIA_TRY_VOID(to->iter_btss_skip_fw(size));

        auto from_path      = self.path();
        Position from_pos   = Position(self.iter_local_pos());

        auto to_path        = to->path();
        Position to_pos     = Position(to->iter_local_pos());

        Position sizes;

        MEMORIA_TRY_VOID(
            self.ctr().ctr_remove_entries(from_path, from_pos, to_path, to_pos, sizes, true)
        );

        self.iter_local_pos() = to_pos.get();

        //MEMORIA_TRY_VOID(self.iter_refresh());

        return ResultT::of(sizes[0]);
    }

    Result<CtrSizeT> iter_remove_to(IteratorPtr to) noexcept
    {
        auto& self = this->self();

        auto from_path      = self.path();
        Position from_pos   = Position(self.iter_local_pos());

        auto to_path        = to.path();
        Position to_pos     = Position(to.iter_local_pos());

        Position sizes;

        self.ctr().ctr_remove_entries(from_path, from_pos, to_path, to_pos, sizes, true);

        self.iter_local_pos() = to_pos.get();
        self.iter_refresh();

        return Result<CtrSizeT>::of(sizes[0]);
    }


    auto iter_bulk_insert(btss::io::IOVectorBTSSInputProvider<Container>& provider) noexcept
    {
        auto& self = this->self();
        return self.ctr().insert(self, provider);
    }


    template <typename Iterator>
    class EntryAdaptor {
        Iterator current_;
    public:
        EntryAdaptor(const Iterator& current): current_(current) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<0>, V&& entry) {}

        template <int32_t SubstreamIdx, typename V>
        void put(StreamTag<0>, StreamTag<SubstreamIdx>, int32_t block, V&& entry) {
            *current_ = entry;
        }

        void next() {
            current_++;
        }
    };

    template <typename OutputIterator>
    Result<CtrSizeT> iter_read_entries(OutputIterator begin, CtrSizeT length) noexcept
    {
        auto& self = this->self();

        EntryAdaptor<OutputIterator> adaptor(begin);

        return self.ctr().template ctr_read_entries<0>(self, length, adaptor);
    }


    auto insert_iovector(io::IOVectorProducer& producer, int64_t start, int64_t length) noexcept
    {
        auto& self = this->self();
        return self.ctr().ctr_insert_iovector(self, producer, start, length);
    }

    auto insert_iovector(io::IOVector& io_vector, int64_t start, int64_t length) noexcept
    {
        auto& self = this->self();
        return self.ctr().ctr_insert_iovector(self, io_vector, start, length);
    }

    auto read_to(io::IOVectorConsumer& consumer, int64_t length) noexcept
    {
        auto& self = this->self();

        std::unique_ptr<io::IOVector> iov = Types::LeafNode::create_iovector();

        auto processed = self.populate(*iov.get(), length);

        consumer.consume(*iov.get());

        return processed;
    }

    auto populate(io::IOVector& io_vector, int64_t length) noexcept
    {
        auto& self = this->self();

        auto& view = self.iovector_view();

        int64_t processed{};

        while (processed < length)
        {
            int32_t start = self.iter_local_pos();
            int32_t size  = self.iter_leaf_size();

            int32_t leaf_remainder = size - start;
            int32_t to_copy;

            if (processed + leaf_remainder <= length)
            {
                to_copy = leaf_remainder;
            }
            else {
                to_copy = length - processed;
            }

            view.symbol_sequence().copy_to(io_vector.symbol_sequence(), start, to_copy);

            for (int32_t ss = 0; ss < view.substreams(); ss++)
            {
                view.substream(ss).copy_to(io_vector.substream(ss), start, to_copy);
            }

            processed += to_copy;

            if (!self.iter_next_leaf()) {
                break;
            }
        }

        io_vector.reindex();

        return processed;
    }

public:

    int32_t iter_data_stream_s() const noexcept {
        return 0;
    }

    Result<SplitResult> iter_split_leaf(int32_t stream, int32_t target_idx) noexcept
    {
        using ResultT = Result<SplitResult>;
        auto& self = this->self();

        int32_t& idx = self.iter_local_pos();

        MEMORIA_TRY(size, self.iter_leaf_size(0));
        int32_t split_idx = size/2;

        MEMORIA_TRY_VOID(self.ctr().ctr_split_leaf(self.path(), Position::create(0, split_idx)));

        if (idx > split_idx)
        {
            MEMORIA_TRY_VOID(self.ctr().ctr_get_next_node(self.path(), 0));
            idx -= split_idx;
            MEMORIA_TRY_VOID(self.iter_refresh());
            self.refresh_iovector_view();
        }

        if (target_idx > split_idx)
        {
            return ResultT::of(SplitStatus::RIGHT, target_idx - split_idx);
        }
        else {
            return ResultT::of(SplitStatus::LEFT, target_idx);
        }
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btss::IteratorMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}
