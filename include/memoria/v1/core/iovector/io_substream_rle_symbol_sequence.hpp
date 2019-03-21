
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

#include <memoria/v1/core/iovector/io_symbol_sequence_base.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <functional>

namespace memoria {
namespace v1 {
namespace io {

template <int32_t AlphabetSize>
class PackedSymbolSequenceImpl: public IOSymbolSequence {

    using SeqT = PkdRLESeqT<AlphabetSize>;

    SeqT* sequence_;

public:
    PackedSymbolSequenceImpl()
    {
        sequence_ = T2T<SeqT*>(allocate_system<uint8_t>(SeqT::empty_size()).release());
        sequence_->allocatable().setTopLevelAllocator();
        (void)sequence_->init();
    }

    PackedSymbolSequenceImpl(PackedSymbolSequenceImpl&&) = delete;
    PackedSymbolSequenceImpl(const PackedSymbolSequenceImpl&) = delete;

    virtual ~PackedSymbolSequenceImpl() noexcept
    {
        free_system(sequence_);
    }

    virtual bool is_indexed() const {
        return true;
    }

    virtual int32_t alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return false;
    }

    virtual int32_t symbol(int32_t idx) const {
        return sequence_->symbol(idx);
    }

    virtual int32_t size() const {
        return sequence_->size();
    }

    virtual void* buffer() const {
        return sequence_;
    }

    virtual void rank_to(int32_t idx, int32_t* values) const
    {
        for (int32_t sym = 0; sym < AlphabetSize; sym++) {
            values[sym] = sequence_->rank(idx, sym);
        }
    }

    virtual void append(int32_t symbol, int32_t length)
    {
        while (sequence_->append(symbol, length) != OpStatus::OK)
        {
            enlarge();
        }
    }

    virtual void reindex()
    {
        while (sequence_->reindex() != OpStatus::OK) {
            enlarge();
        }
    }

    virtual void reset() {
        sequence_->reset();
    }

    virtual void dump(std::ostream& out) const
    {
        sequence_->dump(out, true);
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }

    virtual void init(void* ptr) {
        MMA1_THROW(UnsupportedOperationException());
    }

private:

    void enlarge()
    {
        int32_t bs = sequence_->block_size();
        SeqT* new_seq = T2T<SeqT*>(allocate_system<uint8_t>(bs * 2).release());
        memcpy(new_seq, sequence_, bs);
        new_seq->set_block_size(bs * 2);

        free_system(sequence_);

        sequence_ = new_seq;
    }
};


template <>
class PackedSymbolSequenceImpl<1>: public IOSymbolSequence {

    int32_t size_{};

public:
    PackedSymbolSequenceImpl()
    {
    }

    PackedSymbolSequenceImpl(PackedSymbolSequenceImpl&&) = delete;
    PackedSymbolSequenceImpl(const PackedSymbolSequenceImpl&) = delete;


    virtual bool is_indexed() const {
        return true;
    }

    virtual int32_t alphabet_size() const {
        return 1;
    }

    virtual bool is_const() const {
        return false;
    }

    virtual int32_t symbol(int32_t idx) const {
        return 0;
    }

    virtual int32_t size() const {
        return size_;
    }

    virtual void* buffer() const {
        return nullptr;
    }

    virtual void rank_to(int32_t idx, int32_t* values) const
    {
        values[0] = idx;
    }

    virtual void append(int32_t symbol, int32_t length)
    {
        size_ += length;
    }

    virtual void reindex()
    {
    }

    virtual void dump(std::ostream& out) const
    {
        out << "[SingleBitSymbolSequence: " << size_ << "]";
    }

    virtual void reset() {
        size_ = 0;
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(PackedSymbolSequenceImpl<1>);
    }

    virtual void init(void* ptr) {
        MMA1_THROW(UnsupportedOperationException());
    }
};



static inline std::unique_ptr<IOSymbolSequence> make_packed_owning_symbol_sequence(int32_t alphabet_size)
{
    if (alphabet_size == 1) {
        return std::make_unique<PackedSymbolSequenceImpl<1>>();
    }
    else if (alphabet_size == 2) {
        return std::make_unique<PackedSymbolSequenceImpl<2>>();
    }
    else if (alphabet_size == 3) {
        return std::make_unique<PackedSymbolSequenceImpl<3>>();
    }
    else if (alphabet_size == 4) {
        return std::make_unique<PackedSymbolSequenceImpl<4>>();
    }
    else if (alphabet_size == 5) {
        return std::make_unique<PackedSymbolSequenceImpl<5>>();
    }
    else if (alphabet_size == 6) {
        return std::make_unique<PackedSymbolSequenceImpl<6>>();
    }
    else if (alphabet_size == 7) {
        return std::make_unique<PackedSymbolSequenceImpl<7>>();
    }
    else {
        MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Unsupported alphabe_size value: {}", alphabet_size);
    }
}

}}}
