
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

#include <memoria/core/iovector/io_symbol_sequence_base.hpp>
#include <memoria/core/packed/sseq/packed_rle_searchable_seq.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/iovector/io_substream_rle_symbol_sequence_view_1.hpp>

#include <functional>

namespace memoria {
namespace io {

template <int32_t AlphabetSize>
class PackedRLESymbolSequenceView final: public IOSymbolSequence {

    using SeqT = PkdRLESeqT<AlphabetSize>;

    const SeqT* sequence_;

public:
    PackedRLESymbolSequenceView(): sequence_()
    {
    }

    PackedRLESymbolSequenceView(PackedRLESymbolSequenceView&&) = delete;
    PackedRLESymbolSequenceView(const PackedRLESymbolSequenceView&) = delete;

    virtual ~PackedRLESymbolSequenceView() noexcept {}

    virtual bool is_indexed() const {
        return true;
    }

    virtual int32_t alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return true;
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
        MMA_THROW(UnsupportedOperationException());
    }

    virtual void rank_to(uint64_t idx, uint64_t* values) const
    {
        for (int32_t sym = 0; sym < AlphabetSize; sym++) {
            values[sym] = sequence_->rank(idx, sym);
        }
    }

    virtual void append(int32_t symbol, uint64_t length)
    {
        MMA_THROW(RuntimeException()) << WhatCInfo("Appending is not supported for PackedRLESymbolSequenceView");
    }

    virtual void reindex()
    {
        MMA_THROW(RuntimeException()) << WhatCInfo("Reindexing is not supported for PackedRLESymbolSequenceView");
    }

    virtual void reset() {
        MMA_THROW(RuntimeException()) << WhatCInfo("Resetting is not supported for PackedRLESymbolSequenceView");
    }

    virtual void dump(std::ostream& out) const
    {
        sequence_->dump(out, true);
    }

    const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }

    void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        IOSymbolSequence& target_stream = substream_cast<IOSymbolSequence>(target);
        target_stream.append_from(*this, start, length);
    }

    void append_from(const IOSymbolSequence& source, int32_t start, int32_t length)
    {
        MMA_THROW(RuntimeException()) << WhatCInfo("Appending from is not supported for PackedRLESymbolSequenceView");
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


    void configure(const void* ptr)
    {
        sequence_ = T2T<const SeqT*>(ptr);
    }
};

}}
