
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/prototypes/bt/nodes/branch_node.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_tree_path.hpp>

#include <memoria/prototypes/bt/tools/bt_tools_iovector.hpp>

namespace memoria {
namespace btss {
namespace io {

using bt::LeafNode;
using bt::StreamTag;


template <typename CtrT>
class IOVectorBTSSInputProviderBase: public bt::IOVectorInputProviderBase<CtrT> {
    using Base = bt::IOVectorInputProviderBase<CtrT>;
public:
    using MyType = IOVectorBTSSInputProviderBase<CtrT>;
    using TreeNodePtr = typename CtrT::Types::TreeNodePtr;
    using TreePathT = TreePath<TreeNodePtr>;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename CtrT::Types::Position;
    using NodePair  = std::pair<TreeNodePtr, TreeNodePtr>;

protected:
    uint64_t start_ = 0;
    uint64_t size_ = 0;
    bool finished_ = false;

    CtrT& ctr_;

    CtrSizeT total_symbols_{};

    memoria::io::IOVectorProducer* producer_{};
    memoria::io::IOVector* io_vector_{};

    CtrSizeT start_pos_;
    CtrSizeT length_;

    bool reset_iovector_;

public:

    IOVectorBTSSInputProviderBase(
            CtrT& ctr,
            memoria::io::IOVectorProducer* producer,
            memoria::io::IOVector* io_vector,
            CtrSizeT start_pos,
            CtrSizeT length,
            bool reset_iovector
    ):
        ctr_(ctr),
        producer_(producer), io_vector_(io_vector),
        start_pos_(start_pos), length_(length),
        reset_iovector_(reset_iovector)
    {

    }

    virtual ~IOVectorBTSSInputProviderBase()  {}

    CtrT& ctr() {return ctr_;}
    const CtrT& ctr() const {return ctr_;}

    CtrSizeT totals() const {
        return total_symbols_;
    }

    virtual bool hasData()
    {
        bool buffer_has_data = start_ < size_;
        auto res = populate_buffer();
        return buffer_has_data || res;
    }

    virtual Position fill(const TreeNodePtr& leaf, const Position& from)
    {
        Position pos = from;

        while(true)
        {
            auto buffer_sizes = buffer_size();

            if (buffer_sizes == 0)
            {
                auto res = populate_buffer();
                if (!res)
                {
                    return pos;
                }
                else {
                    buffer_sizes = buffer_size();
                }
            }

            auto capacity = findCapacity(leaf, buffer_sizes);

            if (capacity > 0)
            {
                insertBuffer(leaf, pos[0], capacity);

                auto rest = buffer_size();

                if (rest > 0)
                {
                    return Position{pos[0] + capacity};
                }
                else {
                    pos[0] += capacity;
                }
            }
            else {
                return pos;
            }
        }
    }

    virtual size_t findCapacity(const TreeNodePtr& leaf, size_t size) = 0;

    struct PrepareInsertBufferFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        size_t current_substream{};
        PkdUpdateStatus status{PkdUpdateStatus::SUCCESS};

        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, int32_t StreamsStartIdx,
                typename StreamObj, typename UpdateState
        >
        void stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                IntList<StreamsStartIdx>,
                size_t at,
                size_t start,
                size_t size,
                memoria::io::IOVector& io_vector,
                UpdateState& update_state
        ){
            if (is_success(status))
            {
                status = stream.prepare_insert_io_substream(
                            at,
                            io_vector.substream(current_substream),
                            start,
                            size,
                            std::get<AllocatorIdx - StreamsStartIdx>(update_state)
                );
                current_substream++;
            }
        }


        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, int32_t StreamsStartIdx,
                typename ExtData, typename PkdStruct, typename UpdateState
        >
        void stream(
                PackedSizedStructSO<ExtData, PkdStruct>& stream,
                PackedAllocator* alloc,
                IntList<StreamsStartIdx>,
                size_t at,
                size_t start,
                size_t size,
                memoria::io::IOVector& io_vector,
                UpdateState& update_state
        ){
            // Do nothing here
        }

        template <typename LCtrT, typename NodeT, typename... Args>
        void treeNode(LeafNodeSO<LCtrT, NodeT>& leaf, Args&&... args) {
            constexpr int32_t StreamsStartIdx = NodeT::StreamsStart;
            return leaf.processSubstreamGroups(*this, leaf.allocator(), IntList<StreamsStartIdx>{}, std::forward<Args>(args)...);
        }
    };



    struct CommitInsertBufferFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        size_t current_substream{};

        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, int32_t StreamsStartIdx,
                typename StreamObj, typename UpdateState
        >
        void stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                IntList<StreamsStartIdx>,
                size_t at,
                size_t start,
                size_t size,
                memoria::io::IOVector& io_vector,
                UpdateState& update_state
        ){
            stream.commit_insert_io_substream(
                            at,
                            io_vector.substream(current_substream),
                            start,
                            size,
                            std::get<AllocatorIdx - StreamsStartIdx>(update_state)
            );
            current_substream++;
        }


        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, int32_t StreamsStartIdx,
                typename ExtData, typename PkdStruct, typename UpdateState
        >
        void stream(
                PackedSizedStructSO<ExtData, PkdStruct>& stream,
                PackedAllocator*,
                IntList<StreamsStartIdx>,
                size_t at,
                size_t,
                size_t size,
                memoria::io::IOVector&,
                UpdateState&
        ){
            stream.insert_space(at, size);
        }

        template <typename LCtrT, typename NodeT, typename... Args>
        void treeNode(LeafNodeSO<LCtrT, NodeT>& leaf, Args&&... args) {
            constexpr int32_t StreamsStartIdx = NodeT::StreamsStart;
            return leaf.processSubstreamGroups(*this, leaf.allocator(), IntList<StreamsStartIdx>{}, std::forward<Args>(args)...);
        }
    };


    virtual size_t insertBuffer(const TreeNodePtr& leaf, size_t at, size_t size)
    {
        auto update_state = ctr().template ctr_make_leaf_update_state<IntList<>>(leaf.as_immutable());

        CommitInsertBufferFn fn;
        ctr().leaf_dispatcher().dispatch(leaf, fn, at, start_, size, *io_vector_, update_state);

        start_ += size;
        total_symbols_ += size;
        return size;
    }

    uint64_t buffer_size() const
    {
        return size_ - start_;
    }

    virtual bool populate_buffer()
    {
        if (start_ < size_)
        {
            return true;
        }
        else if (!finished_)
        {
            do_populate_iobuffer();
            if (finished_)
            {
                return start_ < size_;
            }
            else {
                return true;
            }
        }
        else {
            return false;
        }
    }

    void do_populate_iobuffer()
    {
        auto& seq = io_vector_->symbol_sequence();

        do
        {
            start_ = 0;
            size_ = 0;

            if (MMA_LIKELY(reset_iovector_)) {
                io_vector_->clear();
            }

            finished_ = producer_->populate(*io_vector_);

            if (MMA_LIKELY(reset_iovector_)) {
                io_vector_->reindex();
            }

            seq.rank_to(io_vector_->symbol_sequence().size(), unit_span_of(&size_));

            if (MMA_UNLIKELY(start_pos_ > 0))
            {
                size_t ctr_seq_size = seq.size();
                if (start_pos_ < ctr_seq_size)
                {
                    seq.rank_to(start_pos_, unit_span_of(&start_));
                    start_pos_ -= start_;
                }
                else {
                    start_pos_ -= ctr_seq_size;
                }
            }
        }
        while (start_pos_ > 0);

        CtrSizeT remainder = length_ - total_symbols_;
        if (MMA_UNLIKELY(length_ < std::numeric_limits<CtrSizeT>::max() && (size_ - start_) > remainder))
        {
            seq.rank_to(start_ + remainder, unit_span_of(&size_));
            finished_ = true;
        }    
    }
};





template <
    typename CtrT,
    LeafDataLengthType LeafDataLength = CtrT::Types::LeafDataLength
>
class IOVectorBTSSInputProvider;




template <typename CtrT>
class IOVectorBTSSInputProvider<CtrT, LeafDataLengthType::FIXED>: public IOVectorBTSSInputProviderBase<CtrT> {
    using Base = IOVectorBTSSInputProviderBase<CtrT>;

public:
    using TreeNodePtr = typename CtrT::Types::TreeNodePtr;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename Base::Position;

public:

    IOVectorBTSSInputProvider(
            CtrT& ctr,
            memoria::io::IOVectorProducer* producer,
            memoria::io::IOVector* io_vector,
            CtrSizeT start_pos,
            CtrSizeT length,
            bool reset_iovector = true
    ): Base(ctr, producer, io_vector, start_pos, length, reset_iovector)
    {}

    virtual size_t findCapacity(const TreeNodePtr& leaf, size_t size)
    {
        auto capacity = this->ctr_.ctr_get_leaf_node_capacity(leaf.as_immutable());

        if (capacity > size)
        {
            return size;
        }
        else {
            return capacity;
        }
    }
};




template <typename CtrT>
class IOVectorBTSSInputProvider<CtrT, LeafDataLengthType::VARIABLE>: public IOVectorBTSSInputProviderBase<CtrT> {
    using Base = IOVectorBTSSInputProviderBase<CtrT>;

public:
    using TreeNodePtr = typename CtrT::Types::TreeNodePtr;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename Base::Position;

    using Base::io_vector_;
    using Base::ctr;

public:

    IOVectorBTSSInputProvider(
            CtrT& ctr,
            memoria::io::IOVectorProducer* producer,
            memoria::io::IOVector* io_vector,
            CtrSizeT start_pos,
            CtrSizeT length,
            bool reset_iovector = true
    ): Base(ctr, producer, io_vector, start_pos, length, reset_iovector)
    {}

    virtual Position fill(const TreeNodePtr& leaf, const Position& from)
    {
        size_t pos = from[0];

        while(true)
        {
            auto has_data = this->hasData();

            if (!has_data) {
                break;
            }

            auto buffer_sizes = this->buffer_size();

            auto inserted = insertBuffer(leaf, pos, buffer_sizes);
            if (inserted > 0)
            {
                pos += inserted;

                if (getFreeSpacePart(leaf) < 0.05)
                {
                    break;
                }
            }
            else {
                break;
            }
        }

        return Position{pos};
    }

    virtual size_t insertBuffer(const TreeNodePtr& leaf, size_t at, size_t size)
    {
        auto inserted = this->insertBuffer_(leaf, at, size);

        this->total_symbols_ += inserted;

        return inserted;
    }

    size_t insertBuffer_(const TreeNodePtr& leaf, size_t at, size_t size)
    {
        auto ins_result = tryInsertBuffer(leaf, at, size);
        if (ins_result)
        {
            this->start_ += size;
            return size;
        }
        else {
            auto imax = size;
            decltype(imax) imin  = 0;
            decltype(imax) start = 0;

            size_t accepts = 0;

            while (imax > imin && (getFreeSpacePart(leaf) > 0.05))
            {
                if (imax - 1 != imin)
                {
                    auto mid = imin + ((imax - imin) / 2);

                    size_t try_block_size = mid - start;

                    auto ins_result2 = tryInsertBuffer(leaf, at, try_block_size);
                    if (ins_result2)
                    {
                        imin = mid + 1;

                        start = mid;
                        at += try_block_size;
                        this->start_ += try_block_size;

                        accepts = 0;
                    }
                    else {
                        imax = mid - 1;
                    }
                }
                else {
                    auto ins_result3 = tryInsertBuffer(leaf, at, 1);
                    if (ins_result3)
                    {
                        start += 1;
                        at += 1;
                        this->start_ += 1;
                    }

                    break;
                }
            }
            return start;
        }
    }

protected:
    virtual size_t findCapacity(const TreeNodePtr& leaf, size_t size)  {
        return 0;
    }


    bool tryInsertBuffer(const TreeNodePtr& leaf, size_t at, size_t size)
    {
        auto update_state = ctr().template ctr_make_leaf_update_state<IntList<>>(leaf.as_immutable());

        typename Base::PrepareInsertBufferFn fn1;
        ctr().leaf_dispatcher().dispatch(leaf, fn1, at, this->start_, size, *io_vector_, update_state);

        if (is_success(fn1.status)) {
            typename Base::CommitInsertBufferFn fn2;
            ctr().leaf_dispatcher().dispatch(leaf, fn2, at, this->start_, size, *io_vector_, update_state);
            return true;
        }

        return true;
    }

    float getFreeSpacePart(const TreeNodePtr& node)
    {
        float client_area = node->allocator()->client_area();
        float free_space = node->allocator()->free_space();

        return free_space / client_area;
    }
};


}}}
