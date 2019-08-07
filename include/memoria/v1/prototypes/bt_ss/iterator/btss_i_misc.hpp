
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

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_input_iovector.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>

namespace memoria {
namespace v1 {

using bt::StreamTag;

MEMORIA_V1_ITERATOR_PART_BEGIN(btss::IteratorMiscName)

    using typename Base::NodeBaseG;
    using Container = typename Base::Container;

    typedef typename Container::Allocator                                           Allocator;
    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Container::Iterator                                            Iterator;

    using Position = typename Container::Types::Position;
    using CtrSizeT = typename Container::Types::CtrSizeT;



public:
    int32_t iovector_pos() const {
        return self().local_pos();
    }

    bool next_leaf() {
        return self().nextLeaf();
    }

    bool next_entry() {
        return self().skipFw(1) > 0;
    }


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

        auto from_path      = self.leaf();
        Position from_pos   = Position(self.local_pos());

        auto to_path        = to.leaf();
        Position to_pos     = Position(to.local_pos());

        Position sizes;

        self.ctr().removeEntries(from_path, from_pos, to_path, to_pos, sizes, true);

        self.local_pos() = to_pos.get();

        self.refresh();

        return sizes[0];
    }


    auto bulk_insert(btss::io::IOVectorBTSSInputProvider<Container>& provider)
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
    CtrSizeT read(OutputIterator begin, CtrSizeT length)
    {
        auto& self = this->self();

        EntryAdaptor<OutputIterator> adaptor(begin);

        return self.ctr().template read_entries<0>(self, length, adaptor);
    }


    auto insert_iovector(io::IOVectorProducer& producer, int64_t start, int64_t length)
    {
        auto& self = this->self();
        return self.ctr().insert_iovector(self, producer, start, length);
    }

    auto insert_iovector(io::IOVector& io_vector, int64_t start, int64_t length)
    {
        auto& self = this->self();
        return self.ctr().insert_iovector(self, io_vector, start, length);
    }

    auto read_to(io::IOVectorConsumer& consumer, int64_t length)
    {
        auto& self = this->self();

        std::unique_ptr<io::IOVector> iov = Types::LeafNode::create_iovector();

        auto processed = self.populate(*iov.get(), length);

        consumer.consume(*iov.get());

        return processed;
    }

    auto populate(io::IOVector& io_vector, int64_t length)
    {
        auto& self = this->self();

        auto& view = self.iovector_view();

        int64_t processed{};

        while (processed < length)
        {
            int32_t start = self.local_pos();
            int32_t size  = self.leaf_size();

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

            if (!self.nextLeaf()) {
                break;
            }
        }

        io_vector.reindex();

        return processed;
    }

public:

    SplitResult split(int32_t stream, int32_t target_idx)
    {
        auto& self = this->self();

        NodeBaseG& leaf     = self.leaf();
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

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btss::IteratorMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}}
