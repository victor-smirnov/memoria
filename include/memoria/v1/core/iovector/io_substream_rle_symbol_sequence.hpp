
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


#include <memoria/v1/core/iovector/io_substream_rle_symbol_sequence_1.hpp>

#include <functional>

namespace memoria {
namespace v1 {
namespace io {

template <int32_t AlphabetSize>
class PackedRLESymbolSequence: public IOSymbolSequence {

    using SeqT = PkdRLESeqT<AlphabetSize>;

    SeqT* sequence_;

public:
    PackedRLESymbolSequence()
    {
        sequence_ = T2T<SeqT*>(allocate_system<uint8_t>(SeqT::empty_size()).release());
        sequence_->allocatable().setTopLevelAllocator();
        (void)sequence_->init();
    }

    PackedRLESymbolSequence(PackedRLESymbolSequence&&) = delete;
    PackedRLESymbolSequence(const PackedRLESymbolSequence&) = delete;

    virtual ~PackedRLESymbolSequence() noexcept
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

    virtual int32_t symbol(uint64_t idx) const {
        return sequence_->symbol(idx);
    }

    virtual uint64_t size() const {
        return sequence_->size();
    }

    virtual const void* buffer() const {
        return sequence_;
    }

    virtual void* buffer() {
        return sequence_;
    }

    virtual void rank_to(uint64_t idx, uint64_t* values) const
    {
        for (int32_t sym = 0; sym < AlphabetSize; sym++) {
            values[sym] = sequence_->rank(idx, sym);
        }
    }

    virtual void append(int32_t symbol, uint64_t length)
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

    virtual void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        IOSymbolSequence& target_stream = substream_cast<IOSymbolSequence>(target);
        target_stream.append_from(*this, start, length);
    }

    void append_from(const IOSymbolSequence& source, int32_t start, int32_t length)
    {
        SeqT* source_seq = T2T<SeqT*>(source.buffer());

        while (sequence_->insert_from(sequence_->size(), source_seq, start, length) != OpStatus::OK) {
            enlarge();
        }
    }

    uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx) const
    {
        return sequence_->populate(buffer, idx);
    }

    uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx, uint64_t size) const
    {
        return sequence_->populate(buffer, idx, size);
    }

    virtual uint64_t populate_buffer_while(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol) const
    {
        return sequence_->populate_while(buffer, idx, symbol);
    }

    virtual uint64_t populate_buffer_entry(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol, bool entry_start) const
    {
        return sequence_->populate_entry(buffer, idx, symbol, entry_start);
    }


    void configure(const void* ptr) {
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







static inline std::unique_ptr<IOSymbolSequence> make_packed_rle_symbol_sequence(int32_t alphabet_size)
{
    if (alphabet_size == 1) {
        return std::make_unique<PackedRLESymbolSequence<1>>();
    }
    else if (alphabet_size == 2) {
        return std::make_unique<PackedRLESymbolSequence<2>>();
    }
    else if (alphabet_size == 3) {
        return std::make_unique<PackedRLESymbolSequence<3>>();
    }
    else if (alphabet_size == 4) {
        return std::make_unique<PackedRLESymbolSequence<4>>();
    }
    else if (alphabet_size == 5) {
        return std::make_unique<PackedRLESymbolSequence<5>>();
    }
    else if (alphabet_size == 6) {
        return std::make_unique<PackedRLESymbolSequence<6>>();
    }
    else if (alphabet_size == 7) {
        return std::make_unique<PackedRLESymbolSequence<7>>();
    }
    else {
        MMA1_THROW(RuntimeException()) << format_ex("Unsupported alphabe_size value: {}", alphabet_size);
    }
}

}}}
