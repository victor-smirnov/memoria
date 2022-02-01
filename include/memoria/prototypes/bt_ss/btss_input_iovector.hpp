
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


namespace memoria {
namespace btss {
namespace io {

using bt::LeafNode;
using bt::StreamTag;


template <typename CtrT>
class IOVectorBTSSInputProviderBase {

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

    virtual ~IOVectorBTSSInputProviderBase() noexcept {}

    CtrT& ctr() {return ctr_;}
    const CtrT& ctr() const {return ctr_;}

    CtrSizeT totals() const {
        return total_symbols_;
    }

    void iter_next_leaf(const TreeNodePtr& leaf) {
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

    virtual int32_t findCapacity(const TreeNodePtr& leaf, int32_t size) = 0;

    struct InsertBufferFn
    {
        enum class StreamType {DATA, STRUCTURE};

        template <StreamType T> struct Tag {};

        int32_t current_substream_{};

        template <int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx, typename StreamObj>
        VoidResult stream(
                StreamObj&& stream,
                PackedAllocator* alloc,
                int32_t at,
                int32_t start,
                int32_t size,
                memoria::io::IOVector& io_vector) noexcept
        {
            MEMORIA_TRY_VOID(stream.insert_io_substream(
                    at,
                    io_vector.substream(current_substream_),
                    start,
                    size
            ));

            current_substream_++;

            return VoidResult::of();
        }

        template <
                int32_t StreamIdx, int32_t AllocatorIdx, int32_t Idx,
                typename ExtData, typename PkdStruct
        >
        VoidResult stream(
                PackedSizedStructSO<ExtData, PkdStruct>& stream,
                PackedAllocator* alloc,
                int32_t at,
                int32_t start,
                int32_t size,
                memoria::io::IOVector& io_vector) noexcept
        {
            return stream.insertSpace(at, size);
        }

        template <typename LCtrT, typename NodeT, typename... Args>
        VoidResult treeNode(LeafNodeSO<LCtrT, NodeT>& leaf, Args&&... args) noexcept
        {
            MEMORIA_TRY_VOID(leaf.layout(255));
            return leaf.processSubstreamGroups(*this, leaf.allocator(), std::forward<Args>(args)...);
        }
    };


    virtual void insertBuffer(const TreeNodePtr& leaf, int32_t at, int32_t size)
    {
        InsertBufferFn fn;
        ctr().leaf_dispatcher().dispatch(leaf, fn, at, start_, size, *io_vector_).get_or_throw();

        start_ += size;

        total_symbols_ += size;
    }

    int32_t buffer_size() const noexcept
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
                int32_t ctr_seq_size = seq.size();
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

    virtual int32_t findCapacity(const TreeNodePtr& leaf, int32_t size)
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

    using BlockUpdateMgr     = typename CtrT::Types::BlockUpdateMgr;

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
        int32_t pos = from[0];

        BlockUpdateMgr mgr(this->ctr());

        mgr.add(leaf);

        while(true)
        {
            auto has_data = this->hasData();

            if (!has_data) {
                break;
            }

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

        return Position{pos};
    }

    virtual int32_t insertBuffer(BlockUpdateMgr& mgr, const TreeNodePtr& leaf, int32_t at, int32_t size)
    {
        auto inserted = this->insertBuffer_(mgr, leaf, at, size);

        this->total_symbols_ += inserted;

        return inserted;
    }

    int32_t insertBuffer_(BlockUpdateMgr& mgr, const TreeNodePtr& leaf, int32_t at, int32_t size)
    {
        auto ins_result = tryInsertBuffer(mgr, leaf, at, size);

        if (ins_result)
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

                    auto ins_result2 = tryInsertBuffer(mgr, leaf, at, try_block_size);
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
                    auto ins_result3 = tryInsertBuffer(mgr, leaf, at, 1);
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
    virtual int32_t findCapacity(const TreeNodePtr& leaf, int32_t size) noexcept {
        return 0;
    }


    bool tryInsertBuffer(BlockUpdateMgr& mgr, const TreeNodePtr& leaf, int32_t at, int32_t size)
    {
        typename Base::InsertBufferFn fn;

        VoidResult status = ctr().leaf_dispatcher().dispatch(leaf, fn, at, this->start_, size, *io_vector_);

        if (status.is_error())
        {
            if (status.is_packed_error())
            {
                mgr.restoreNodeState();
                return false;
            }
            else {
                status.get_or_throw();
            }
        }

        mgr.checkpoint(leaf);
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
