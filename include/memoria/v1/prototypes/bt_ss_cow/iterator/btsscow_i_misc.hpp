
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

#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/types.hpp>
#include <memoria/v1/prototypes/bt_ss/btss_names.hpp>

namespace memoria {
namespace v1 {

using bt::StreamTag;

MEMORIA_V1_ITERATOR_PART_BEGIN(btss_cow::IteratorMiscName)

    using typename Base::NodeBaseG;
    using typename Base::Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using Position = typename Container::Types::Position;
    using CtrSizeT = typename Container::Types::CtrSizeT;



public:
    bool operator++() {
        return self().skipFw(1);
    }

    bool next() {
        return self().skipFw(1);
    }

    bool prev() {
        return self().skipBw(1);
    }

    bool operator--() {
        return self().skipBw(1);
    }

    bool operator++(int) {
        return self().skipFw(1);
    }

    bool operator--(int) {
        return self().skipFw(1);
    }

    CtrSizeT operator+=(CtrSizeT size) {
        return self().skipFw(size);
    }

    CtrSizeT operator-=(CtrSizeT size) {
        return self().skipBw(size);
    }

    int32_t size() const
    {
        return self().leafSize(0);
    }

    bool isEof() const {
        return self().local_pos() >= self().size();
    }

    bool isBof() const {
        return self().local_pos() < 0;
    }

    CtrSizeT skipFw(CtrSizeT amount) {
        return self().template skip_fw_<0>(amount);
    }

    CtrSizeT skipBw(CtrSizeT amount) {
        return self().template skip_bw_<0>(amount);
    }

    CtrSizeT skip(CtrSizeT amount) {
        return self().template skip_<0>(amount);
    }

    CtrSizeT pos() const
    {
        auto& self = this->self();

        return self.local_pos() + self.cache().size_prefix()[0];
    }


    void remove()
    {
        auto& self  = this->self();
        auto& ctr   = self.ctr();

        ctr.removeEntry(self);

        if (self.isEnd())
        {
            self.skipFw(0);
        }
    }

    CtrSizeT remove(CtrSizeT size)
    {
        auto& self = this->self();

        auto to = self;

        to.skipFw(size);

        auto& from_path     = self.leaf();
        Position from_pos   = Position(self.local_pos());

        auto& to_path       = to.leaf();
        Position to_pos     = Position(to.local_pos());

        Position sizes;

        self.ctr().removeEntries(from_path, from_pos, to_path, to_pos, sizes, true);

        self.local_pos() = to_pos.get();

        self.refresh();

        return sizes[0];
    }


    auto bulk_insert(btss::AbstractBTSSInputProvider<Container>& provider)
    {
        auto& self = this->self();
        return self.ctr().insert(self, provider);
    }

    template <typename InputIterator>
    auto bulk_insert(InputIterator begin, InputIterator end)
    {
//        auto& self = this->self();
//
//        btss::BTSSIteratorInputProvider<Container, InputIterator> provider(self.ctr(), begin, end);
//
//        return self.ctr().insert(self, provider);
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
    CtrSizeT read(OutputIterator begin, CtrSizeT length)
    {
        auto& self = this->self();

        EntryAdaptor<OutputIterator> adaptor(begin);

        return self.ctr().template read_entries<0>(self, length, adaptor);
    }



    template <typename IOBuffer>
    auto read_buffer(bt::BufferConsumer<IOBuffer>* consumer, CtrSizeT length)
    {
        auto& self = this->self();

        auto buffer = self.ctr().pools().get_instance(PoolT<ObjectPool<IOBuffer>>()).get_unique(65536);

        return self.ctr().template buffered_read<0>(self, length, *buffer.get(), *consumer);
    }

    template <typename IOBuffer>
    auto read_buffer(bt::BufferConsumer<IOBuffer>* consumer)
    {
        auto& self = this->self();
        return self.read_buffer(consumer, self.ctr().size());
    }

    template <typename IOBuffer>
    auto populate_buffer(IOBuffer* buffer, CtrSizeT length)
    {
        auto& self = this->self();
        return self.ctr().template populate_buffer<0>(self, length, *buffer);
    }

    template <typename IOBuffer>
    auto populate_buffer(IOBuffer* buffer)
    {
        auto& self = this->self();
        return self.populate_buffer(buffer, self.ctr().size());
    }


    template <typename IOBuffer>
    auto insert_iobuffer(bt::BufferProducer<IOBuffer>* producer, int32_t ib_initial_capacity = 10000)
    {
        using InputProvider = btss::IOBufferProducerBTSSInputProvider<Container, IOBuffer>;

        auto buffer = self().ctr().pools().get_instance(PoolT<ObjectPool<IOBuffer>>()).get_unique(65536);

        auto bulk = std::make_unique<InputProvider>(self().ctr(), *buffer.get(), producer, ib_initial_capacity);

        return this->bulk_insert(*bulk.get());
    }



protected:

    SplitResult split(int32_t stream, int32_t target_idx)
    {
        auto& self = this->self();

        NodeBaseG& leaf = self.leaf();
        int32_t& idx        = self.local_pos();

        int32_t size        = self.leaf_size(0);
        int32_t split_idx   = size/2;

        auto right = self.ctr().split_leaf_p(leaf, Position::create(0, split_idx));


        if (idx > split_idx)
        {
            leaf = right;
            idx -= split_idx;

            self.refresh();
        }

        if (target_idx > split_idx)
        {
            return SplitResult(SplitStatus::RIGHT, target_idx - split_idx);
        }
        else {
            return SplitResult(SplitStatus::LEFT, target_idx);
        }
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btss_cow::IteratorMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}}
