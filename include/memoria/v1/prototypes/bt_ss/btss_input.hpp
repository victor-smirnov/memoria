
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
namespace btss      {

using bt::LeafNode;
using bt::StreamTag;

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

protected:
    Int start_ = 0;
    Int size_ = 0;

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
            auto sums = ctr().max(leaf);
            ctr().update_parent(leaf, sums);
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
        this->start_ = 0;
        this->size_ = 0;

        input_buffer_->reset();

        start_buffer(input_buffer_);

        Int size;
        Int position = 0;
        while ((size = get(input_buffer_, position)) > 0)
        {
            position += size;
        }

        input_buffer_->reindex();

        total_ += position;

        size_ = position;

        end_buffer(input_buffer_, position);

        return position > 0;
    }

private:
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
            auto max = this->ctr().max(leaf);
            this->ctr().update_parent(leaf, max);
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



template <typename CtrT, typename InputIterator>
class IteratorBTSSInputProvider: public v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
    using Base = v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

    using CtrSizeT  = typename Base::CtrSizeT;
    using Position  = typename Base::Position;
    using InputBuffer = typename Base::InputBuffer;

    using InputValue = typename InputIterator::value_type;

    InputIterator current_;
    InputIterator end_;



    Int input_start_ = 0;
    Int input_size_ = 0;
    static constexpr Int INPUT_END = 1000;

    InputValue input_value_buffer_[INPUT_END];

public:
    IteratorBTSSInputProvider(CtrT& ctr, InputIterator start, InputIterator end, Int capacity = 10000):
        Base(ctr, capacity),
        current_(start),
        end_(end)
    {}

    virtual Int get(InputBuffer* buffer, Int pos)
    {
        if (input_start_ == input_size_)
        {
            input_start_ = 0;

            for (input_size_ = 0 ;current_ != end_ && input_size_ < INPUT_END; input_size_++, current_++)
            {
                input_value_buffer_[input_size_] = *current_;
            }
        }

        if (input_start_ < input_size_)
        {
            auto inserted = buffer->append_vbuffer(this, input_start_, input_size_ - input_start_);

            input_start_ += inserted;

            return inserted;
        }

        return -1;
    }

    auto buffer(StreamTag<0>, StreamTag<0>, Int idx, Int block) {
        return CtrSizeT();
    }

    const auto& buffer(StreamTag<0>, StreamTag<1>, Int idx, Int block) {
        return std::get<0>(input_value_buffer_[idx]);
    }
};





template <typename CtrT, typename MyType, Int BufferSize = 1000, typename EntryT = typename CtrT::Types::Entry>
class AbstractBufferedBTSSInputProvider: public v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {

    using Base = v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

protected:

    using typename Base::CtrSizeT;
    using typename Base::Position;
    using typename Base::InputBuffer;

    using Entry = EntryT;

    Int input_start_    = 0;
    Int input_size_     = 0;

    static constexpr Int INPUT_END = BufferSize;

    Entry input_value_buffer_[INPUT_END];

public:
    AbstractBufferedBTSSInputProvider(CtrT& ctr, Int capacity = 10000):
        Base(ctr, capacity)
    {}

    virtual Int get(InputBuffer* buffer, Int pos)
    {
        auto& self = this->self();

        if (input_start_ == input_size_)
        {
            input_start_ = 0;

            for (input_size_ = 0; self.has_next() && input_size_ < INPUT_END; input_size_++)
            {
                input_value_buffer_[input_size_] = self.next();
            }
        }

        if (input_start_ < input_size_)
        {
            auto inserted = buffer->append_vbuffer(&self, input_start_, input_size_ - input_start_);

            input_start_ += inserted;

            return inserted;
        }

        return -1;
    }

    MyType& self() {return *static_cast<MyType*>(this);}
    const MyType& self() const {return *static_cast<MyType*>(this);}
};



template <
    typename CtrT,
    typename MyType,
    typename InputIterator,
    Int BufferSize = 1000,
    typename BufferEntry = typename CtrT::Types::Entry
>
class AbstractIteratorBTSSInputProvider: public AbstractBufferedBTSSInputProvider<CtrT, MyType, BufferSize, BufferEntry>
{
    using Base = AbstractBufferedBTSSInputProvider<CtrT, MyType, BufferSize, BufferEntry>;

protected:

    using typename Base::CtrSizeT;

    using InputValue = typename InputIterator::value_type;

    InputIterator current_;
    InputIterator end_;

public:
    AbstractIteratorBTSSInputProvider(CtrT& ctr, InputIterator start, InputIterator end, Int capacity = 10000):
        Base(ctr, capacity),
        current_(start),
        end_(end)
    {}

    bool has_next() {
        return current_ != end_;
    }

    template <typename T>
    auto convert(T&& value) const {
        return value;
    }

    auto next() {
        Incrementer inc(current_);
        return this->self().convert(*current_);
    }

private:
    class Incrementer {
        InputIterator& value_;
    public:
        Incrementer(InputIterator& value): value_(value) {}
        ~Incrementer() {value_++;}
    };
};



template <typename CtrT, typename InputIterator, Int EntryBufferSize = 1000>
class BTSSIteratorInputProvider: public AbstractIteratorBTSSInputProvider<
    CtrT,
    BTSSIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
    InputIterator
>
{
    using Base = AbstractIteratorBTSSInputProvider<
            CtrT,
            BTSSIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
            InputIterator
    >;

public:

    using typename Base::CtrSizeT;

public:
    BTSSIteratorInputProvider(CtrT& ctr, const InputIterator& start, const InputIterator& end, Int capacity = 10000):
        Base(ctr, start, end, capacity)
    {}

    auto buffer(StreamTag<0>, StreamTag<0>, Int idx, Int block) {
        return CtrSizeT();
    }

    const auto& buffer(StreamTag<0>, StreamTag<1>, Int idx, Int block) {
        return Base::input_value_buffer_[idx];
    }
};




}
}}