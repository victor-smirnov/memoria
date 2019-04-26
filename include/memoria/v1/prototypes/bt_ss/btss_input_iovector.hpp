
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>


namespace memoria {
namespace v1 {
namespace btss {
namespace io {

using bt::LeafNode;
using bt::StreamTag;


template <typename CtrT>
class IOVectorBTSSInputProviderBase {

public:
    using MyType = IOVectorBTSSInputProviderBase<CtrT>;
    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename CtrT::Types::Position;
    using NodePair  = std::pair<NodeBaseG, NodeBaseG>;

protected:
    uint64_t start_ = 0;
    uint64_t size_ = 0;
    bool finished_ = false;

    CtrT& ctr_;

    NodePair split_watcher_;

    CtrSizeT total_symbols_{};

    memoria::v1::io::IOVectorProducer* producer_{};
    memoria::v1::io::IOVector* io_vector_{};

    CtrSizeT start_pos_;
    CtrSizeT length_;

    bool reset_iovector_;

public:

    IOVectorBTSSInputProviderBase(
            CtrT& ctr,
            memoria::v1::io::IOVectorProducer* producer,
            memoria::v1::io::IOVector* io_vector,
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

    virtual ~IOVectorBTSSInputProviderBase() noexcept {}

    CtrT& ctr() {return ctr_;}
    const CtrT& ctr() const {return ctr_;}

    CtrSizeT totals() const {
        return total_symbols_;
    }

    NodePair& split_watcher() {
        return split_watcher_;
    }

    const NodePair& split_watcher() const {
        return split_watcher_;
    }

    CtrSizeT orphan_splits() const {
        return 0;
    }

    void nextLeaf(const NodeBaseG& leaf) {}

    virtual bool hasData()
    {
        bool buffer_has_data = start_ < size_;

        return buffer_has_data || populate_buffer();
    }

    virtual Position fill(NodeBaseG& leaf, const Position& from)
    {
        Position pos = from;

        while(true)
        {
            auto buffer_sizes = buffer_size();

            if (buffer_sizes == 0)
            {
                if (!populate_buffer())
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
                OOM_THROW_IF_FAILED(insertBuffer(leaf, pos[0], capacity), MMA1_SRC);

                auto rest = buffer_size();

                if (rest > 0)
                {
                    return Position(pos[0] + capacity);
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

    virtual int32_t findCapacity(const NodeBaseG& leaf, int32_t size) = 0;

    struct InsertBufferFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        int32_t current_substream_{};
        OpStatus status_{OpStatus::OK};

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename StreamObj>
        void stream(
                StreamObj* stream,
                PackedAllocator* alloc,
                int32_t at,
                int32_t start,
                int32_t size,
                memoria::v1::io::IOVector& io_vector)
        {
            if (isOk(status_))
            {
                status_ <<= stream->insert_io_substream(
                    at,
                    io_vector.substream(current_substream_),
                    start,
                    size
                );
            }

            current_substream_++;
        }

        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx,
                typename VV, int32_t Indexes_ = 0, PkdSearchType SearchType_>
        void stream(
                PackedSizedStruct<VV, Indexes_, SearchType_>* stream,
                PackedAllocator* alloc,
                int32_t at,
                int32_t start,
                int32_t size,
                memoria::v1::io::IOVector& io_vector)
        {
            if (isOk(status_))
            {
                status_ <<= stream->insertSpace(at, size);
            }
        }

        template <typename NodeTypes, typename... Args>
        auto treeNode(bt::LeafNode<NodeTypes>* leaf, Args&&... args)
        {
            leaf->layout(255);
            return leaf->processSubstreamGroups(*this, leaf->allocator(), std::forward<Args>(args)...);
        }
    };


    virtual OpStatus insertBuffer(NodeBaseG& leaf, int32_t at, int32_t size)
    {
        InsertBufferFn fn;
        CtrT::Types::Blocks::LeafDispatcher::dispatch(leaf, fn, at, start_, size, *io_vector_);

        if (isFail(fn.status_)) {
            return OpStatus::FAIL;
        }

        start_ += size;

        if (leaf->parent_id().isSet())
        {
            ctr().update_path(leaf);
        }

        return OpStatus::OK;
    }

    int32_t buffer_size() const
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

            if (MMA1_LIKELY(reset_iovector_)) {
                io_vector_->reset();
            }

            finished_ = producer_->populate(*io_vector_);

            if (MMA1_LIKELY(reset_iovector_)) {
                io_vector_->reindex();
            }

            seq.rank_to(io_vector_->symbol_sequence().size(), &size_);

            if (MMA1_UNLIKELY(start_pos_ > 0))
            {
                int32_t seq_size = seq.size();
                if (start_pos_ < seq_size)
                {
                    seq.rank_to(start_pos_, &start_);
                    start_pos_ -= start_;
                }
                else {
                    start_pos_ -= seq_size;
                }
            }
        }
        while (start_pos_ > 0);

        CtrSizeT remainder = length_ - total_symbols_;
        if (MMA1_UNLIKELY(length_ < std::numeric_limits<CtrSizeT>::max() && (size_ - start_) > remainder))
        {
            seq.rank_to(start_ + remainder, &size_);
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
    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename Base::Position;

public:

    IOVectorBTSSInputProvider(
            CtrT& ctr,
            memoria::v1::io::IOVectorProducer* producer,
            memoria::v1::io::IOVector* io_vector,
            CtrSizeT start_pos,
            CtrSizeT length,
            bool reset_iovector = true
    ): Base(ctr, producer, io_vector, start_pos, length, reset_iovector)
    {}

    virtual int32_t findCapacity(const NodeBaseG& leaf, int32_t size)
    {
        int32_t capacity = this->ctr_.getLeafNodeCapacity(leaf);

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
    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename Base::Position;

    using BlockUpdateMgr     = typename CtrT::Types::BlockUpdateMgr;

    using Base::io_vector_;

public:

    IOVectorBTSSInputProvider(
            CtrT& ctr,
            memoria::v1::io::IOVectorProducer* producer,
            memoria::v1::io::IOVector* io_vector,
            CtrSizeT start_pos,
            CtrSizeT length,
            bool reset_iovector = true
    ): Base(ctr, producer, io_vector, start_pos, length, reset_iovector)
    {}

    virtual Position fill(NodeBaseG& leaf, const Position& from)
    {
        int32_t pos = from[0];

        BlockUpdateMgr mgr(this->ctr());

        mgr.add(leaf);

        while(this->hasData())
        {
            auto buffer_sizes = this->buffer_size();

            auto inserted = insertBuffer(mgr, leaf, pos, buffer_sizes);

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

        return Position(pos);
    }

    virtual int32_t insertBuffer(BlockUpdateMgr& mgr, NodeBaseG& leaf, int32_t at, int32_t size)
    {
        int32_t inserted = this->insertBuffer_(mgr, leaf, at, size);

        if (leaf->parent_id().isSet())
        {
            this->ctr().update_path(leaf);
        }

        return inserted;
    }

    int32_t insertBuffer_(BlockUpdateMgr& mgr, NodeBaseG& leaf, int32_t at, int32_t size)
    {
        if (tryInsertBuffer(mgr, leaf, at, size))
        {
            this->start_ += size;
            return size;
        }
        else {
            auto imax = size;
            decltype(imax) imin  = 0;
            decltype(imax) start = 0;

            int32_t accepts = 0;

            while (imax > imin && (getFreeSpacePart(leaf) > 0.05))
            {
                if (imax - 1 != imin)
                {
                    auto mid = imin + ((imax - imin) / 2);

                    int32_t try_block_size = mid - start;
                    if (tryInsertBuffer(mgr, leaf, at, try_block_size))
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
                    if (tryInsertBuffer(mgr, leaf, at, 1))
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
    virtual int32_t findCapacity(const NodeBaseG& leaf, int32_t size) {
        return 0;
    }


    bool tryInsertBuffer(BlockUpdateMgr& mgr, NodeBaseG& leaf, int32_t at, int32_t size)
    {
        typename Base::InsertBufferFn fn;

        CtrT::Types::Blocks::LeafDispatcher::dispatch(leaf, fn, at, this->start_, size, *io_vector_);

        if (isFail(fn.status_))
        {
            mgr.restoreNodeState();
            return false;
        }

        mgr.checkpoint(leaf);
        return true;
    }

    float getFreeSpacePart(const NodeBaseG& node)
    {
        float client_area = node->allocator()->client_area();
        float free_space = node->allocator()->free_space();

        return free_space / client_area;
    }
};


}}}}
