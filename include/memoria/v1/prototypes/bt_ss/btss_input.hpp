
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>

#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>


namespace memoria {
namespace v1 {
namespace btss {

using bt::LeafNode;
using bt::StreamTag;

class BTSSBufferStatus {
    Int size_;
    bool buffer_filled_;
    bool finished_;
public:
    BTSSBufferStatus(Int size = 0, bool buffer_filled = false, bool finished = false):
        size_(size), buffer_filled_(buffer_filled), finished_(finished)
    {}

    Int size() const {return size_;}
    bool is_buffer_filled() const {return buffer_filled_;}
    bool is_finished() const {return finished_;}
};

template <typename CtrT>
class AbstractBTSSInputProviderBase {

public:
    using MyType = AbstractBTSSInputProviderBase<CtrT>;
    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename CtrT::Types::Position;

    using InputBuffer = bt::StreamInputBuffer<
            0,
            typename CtrT::Types::template StreamInputBufferStructList<0>
    >;

    using NodePair = std::pair<NodeBaseG, NodeBaseG>;

    using BufferSizes = typename InputBuffer::BufferSizesT;

protected:
    Int start_ = 0;
    Int size_ = 0;
    bool finish_ = false;

    CtrT&   ctr_;

    NodePair split_watcher_;

    CtrSizeT total_ = 0;

    InputBuffer* input_buffer_;

public:

    AbstractBTSSInputProviderBase(CtrT& ctr, Int capacity):
        ctr_(ctr)
    {
        input_buffer_ = create_input_buffer(capacity);
    }

    ~AbstractBTSSInputProviderBase() {
        delete_buffer(input_buffer_);
    }

    CtrT& ctr() {return ctr_;}
    const CtrT& ctr() const {return ctr_;}

    CtrSizeT total() const {
        return total_;
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
                insertBuffer(leaf, pos[0], capacity);

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

    virtual Int findCapacity(const NodeBaseG& leaf, Int size) = 0;


    struct InsertBufferFn {

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename StreamObj>
        void stream(StreamObj* stream, Int at, const InputBuffer* buffer, Int start, Int size)
        {
            stream->insert_buffer(at, buffer->template substream_by_idx<Idx>(), start, size);
        }

        template <typename NodeTypes, typename... Args>
        void treeNode(LeafNode<NodeTypes>* leaf, Args&&... args)
        {
            leaf->processSubstreamGroups(*this, std::forward<Args>(args)...);
        }
    };


    virtual void insertBuffer(NodeBaseG& leaf, Int at, Int size)
    {
        CtrT::Types::Pages::LeafDispatcher::dispatch(leaf, InsertBufferFn(), at, input_buffer_, start_, size);
        start_ += size;

        if (leaf->parent_id().isSet())
        {
            ctr().update_path(leaf);
        }
    }

    Int buffer_size() const
    {
        return size_ - start_;
    }


    void dump_buffer(std::ostream& out = std::cout) const
    {
        TextPageDumper dumper(std::cout);
        input_buffer_->generateDataEvents(&dumper);
    }

    // must be an abstract method
    virtual Int get(InputBuffer* buffer, Int pos) = 0;

    virtual void start_buffer(InputBuffer* buffer) {}
    virtual void end_buffer(InputBuffer* buffer, Int size) {}

    virtual bool populate_buffer()
    {
        if (size_ == start_)
        {
            if (!finish_)
            {
                start_ = 0;
                size_  = 0;

                input_buffer_->reset();

                start_buffer(input_buffer_);

                Int entries = get(input_buffer_, 0);

                if (entries > 0)
                {
                    size_ = entries;
                }
                else {
                    size_ = -entries;
                    finish_ = true;
                }

                input_buffer_->reindex();

                total_ += this->size_;

                end_buffer(input_buffer_, this->size_);

                return entries != 0;
            }
            else {
                return false;
            }
        }
        else {
            return true;
        }
    }

protected:
    BufferSizes data_capacity() const {
        return input_buffer_->data_capacity();
    }

    static InputBuffer* create_input_buffer(Int capacity)
    {
        Int buffer_size = InputBuffer::block_size(capacity) + 256;

        void* block = malloc(buffer_size);
        if (block != nullptr)
        {
            InputBuffer* buffer = T2T<InputBuffer*>(block);
            buffer->setTopLevelAllocator();
            buffer->init(buffer_size, capacity);

            return buffer;
        }
        else {
            throw OOMException(MA_SRC);
        }
    }

    static InputBuffer* create_input_buffer(const BufferSizes& capacity)
    {
        Int buffer_size = InputBuffer::block_size(capacity) + 256;

        void* block = malloc(buffer_size);
        if (block != nullptr)
        {
            InputBuffer* buffer = T2T<InputBuffer*>(block);
            buffer->setTopLevelAllocator();
            buffer->init(buffer_size, capacity);

            return buffer;
        }
        else {
            throw OOMException(MA_SRC);
        }
    }


    void enlarge()
    {
        BufferSizes current_capacity    = data_capacity();
        BufferSizes new_capacity        = current_capacity;
        VectorAdd(new_capacity, new_capacity);

        auto new_buffer = create_input_buffer(new_capacity);

        input_buffer_->copyTo(new_buffer);

        delete_buffer(input_buffer_);

        input_buffer_ = new_buffer;
    }

    static void delete_buffer(InputBuffer* buffer)
    {
        ::free(buffer);
    }
};





template <
    typename CtrT,
    LeafDataLengthType LeafDataLength = CtrT::Types::LeafDataLength
>
class AbstractBTSSInputProvider;




template <typename CtrT>
class AbstractBTSSInputProvider<CtrT, LeafDataLengthType::FIXED>: public AbstractBTSSInputProviderBase<CtrT> {
    using Base = AbstractBTSSInputProviderBase<CtrT>;

public:
    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename Base::Position;

public:

    AbstractBTSSInputProvider(CtrT& ctr, Int capacity): Base(ctr, capacity) {}

    virtual Int findCapacity(const NodeBaseG& leaf, Int size)
    {
        Int capacity = this->ctr_.getLeafNodeCapacity(leaf);

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
class AbstractBTSSInputProvider<CtrT, LeafDataLengthType::VARIABLE>: public AbstractBTSSInputProviderBase<CtrT> {
    using Base = AbstractBTSSInputProviderBase<CtrT>;

public:
    using NodeBaseG = typename CtrT::Types::NodeBaseG;
    using CtrSizeT  = typename CtrT::Types::CtrSizeT;

    using Position  = typename Base::Position;

    using PageUpdateMgr     = typename CtrT::Types::PageUpdateMgr;

public:

    AbstractBTSSInputProvider(CtrT& ctr, Int capacity): Base(ctr, capacity) {}

    virtual Position fill(NodeBaseG& leaf, const Position& from)
    {
        Int pos = from[0];

        PageUpdateMgr mgr(this->ctr());

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

    virtual Int insertBuffer(PageUpdateMgr& mgr, NodeBaseG& leaf, Int at, Int size)
    {
        Int inserted = this->insertBuffer_(mgr, leaf, at, size);

        if (leaf->parent_id().isSet())
        {
            this->ctr().update_path(leaf);
        }

        return inserted;
    }

    Int insertBuffer_(PageUpdateMgr& mgr, NodeBaseG& leaf, Int at, Int size)
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

            Int accepts = 0;

            while (imax > imin && (getFreeSpacePart(leaf) > 0.05))
            {
                if (imax - 1 != imin)
                {
                    auto mid = imin + ((imax - imin) / 2);

                    Int try_block_size = mid - start;
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
    virtual Int findCapacity(const NodeBaseG& leaf, Int size) {
        return 0;
    }


    bool tryInsertBuffer(PageUpdateMgr& mgr, NodeBaseG& leaf, Int at, Int size)
    {
        try {
            CtrT::Types::Pages::LeafDispatcher::dispatch(leaf, typename Base::InsertBufferFn(), at, this->input_buffer_, this->start_, size);
            mgr.checkpoint(leaf);
            return true;
        }
        catch (PackedOOMException& ex)
        {
            mgr.restoreNodeState();
            return false;
        }
    }

    float getFreeSpacePart(const NodeBaseG& node)
    {
        float client_area = node->allocator()->client_area();
        float free_space = node->allocator()->free_space();

        return free_space / client_area;
    }
};





template <typename CtrT, typename IOBuffer>
class AbstractIOBufferBTSSInputProvider1: public v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {

    using Base = v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

protected:

    using typename Base::CtrSizeT;
    using typename Base::Position;
    using typename Base::InputBuffer;

    IOBuffer& io_buffer_;
    Int num_entries_ = 0;
    Int entry_ = 0;

    Int finish_ = false;

public:
    AbstractIOBufferBTSSInputProvider1(CtrT& ctr, IOBuffer& buffer, Int input_buffer_capacity = 10000):
        Base(ctr, input_buffer_capacity),
        io_buffer_(buffer)
    {}

    virtual ~AbstractIOBufferBTSSInputProvider1() {}

    virtual Int populate(IOBuffer& buffer) = 0;

    virtual Int get(InputBuffer* buffer, Int pos)
    {
        auto input_buffer_state = buffer->append_state();

        Int total = 0;


        while (true)
        {
            if (entry_ == num_entries_ && !finish_)
            {
                io_buffer_.rewind();
                Int entries = populate(io_buffer_);

                if (entries > 0) {
                    num_entries_ = entries;
                }
                else {
                    num_entries_ = -entries;
                    finish_ = true;
                }

                io_buffer_.rewind();
                entry_ = 0;
            }

            for (; entry_ < num_entries_; entry_++, total++)
            {
                auto pos    = io_buffer_.pos();
                auto backup = input_buffer_state;

                if (!buffer->append_entry_from_iobuffer(input_buffer_state, io_buffer_))
                {
                    io_buffer_.pos(pos);
                    input_buffer_state = backup;
                    buffer->restore_append_state(backup);

                    return total;
                }
            }

            if (finish_)
            {
                return -total;
            }
        }
    }
};


template <typename CtrT, typename IOBuffer>
class AbstractIOBufferBTSSInputProvider: public v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {

    using Base = v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

protected:

    using typename Base::CtrSizeT;
    using typename Base::Position;
    using typename Base::InputBuffer;

    using AppendState = typename InputBuffer::AppendState;

    using Base::input_buffer_;


    IOBuffer& io_buffer_;
    Int finished_ = false;

    AppendState append_state_;

public:
    AbstractIOBufferBTSSInputProvider(CtrT& ctr, IOBuffer& buffer, Int input_buffer_capacity = 10000):
        Base(ctr, input_buffer_capacity),
        io_buffer_(buffer)
    {}

    virtual ~AbstractIOBufferBTSSInputProvider() {}

    virtual Int populate(IOBuffer& buffer) = 0;

    virtual Int get(InputBuffer* buffer, Int pos)
    {
        if (!finished_)
        {
            append_state_ = input_buffer_->append_state();

            io_buffer_.rewind();
            Int entries = populate(io_buffer_);

            Int total;
            if (entries >  0)
            {
                total = entries;
            }
            else {
                total = -entries;
                finished_ = true;
            }

            io_buffer_.rewind();

            for (Int c = 0; c < total; c++)
            {
                this->append_io_entry();
            }

            return entries;
        }
        else {
            return 0;
        }
    }



    void append_io_entry(Int enlargements = 0)
    {
        size_t pos = io_buffer_.pos();

        auto tmp = append_state_;

        if (!input_buffer_->append_entry_from_iobuffer(append_state_, io_buffer_))
        {
            append_state_ = tmp;
            io_buffer_.pos(pos);

            if (enlargements < 5)
            {
                this->enlarge();
                append_state_ = input_buffer_->append_state();

                append_io_entry(enlargements + 1);
            }
            else {
                throw Exception(MA_RAW_SRC, "Supplied entry is too large for InputBuffer");
            }
        }
    }
};




template <typename CtrT, typename IOBuffer>
class IOBufferProducerBTSSInputProvider : public AbstractIOBufferBTSSInputProvider<CtrT, IOBuffer> {
    using Base = AbstractIOBufferBTSSInputProvider<CtrT, IOBuffer>;
    bt::BufferProducer<IOBuffer>* producer_;
public:
    IOBufferProducerBTSSInputProvider(CtrT& ctr, bt::BufferProducer<IOBuffer>* producer, Int input_buffer_capacity = 10000):
        Base(ctr, producer->buffer(), input_buffer_capacity),
        producer_(producer)
    {}

    virtual Int populate(IOBuffer& buffer)
    {
        return producer_->populate(buffer);
    }
};

}
}}
