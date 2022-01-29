
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/packed/sseq/packed_ssrle_searchable_seq.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/memory/malloc.hpp>

#include <functional>

namespace memoria {
namespace io {

template <size_t AlphabetSize>
class PackedSSRLESymbolSequenceView final: public IOSubstream {

    using SeqT = PkdSSRLESeqT<AlphabetSize>;

    const SeqT* sequence_;

public:
    PackedSSRLESymbolSequenceView(): sequence_()
    {
    }

    PackedSSRLESymbolSequenceView(PackedSSRLESymbolSequenceView&&) = delete;
    PackedSSRLESymbolSequenceView(const PackedSSRLESymbolSequenceView&) = delete;

    virtual ~PackedSSRLESymbolSequenceView() noexcept {}

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
        MMA_THROW(RuntimeException()) << WhatCInfo("Appending is not supported for PackedSSRLESymbolSequenceView");
    }

    virtual void reindex()
    {
        MMA_THROW(RuntimeException()) << WhatCInfo("Reindexing is not supported for PackedSSRLESymbolSequenceView");
    }

    virtual void reset() {
        MMA_THROW(RuntimeException()) << WhatCInfo("Resetting is not supported for PackedSSRLESymbolSequenceView");
    }

    virtual void dump(std::ostream& out) const
    {
        DumpStruct(sequence_, out);
    }

    const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }

    void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        //IOSymbolSequence& target_stream = substream_cast<IOSymbolSequence>(target);
        //target_stream.append_from(*this, start, length);
    }

    void append_from(const IOSymbolSequence& source, int32_t start, int32_t length)
    {
        //MMA_THROW(RuntimeException()) << WhatCInfo("Appending from is not supported for PackedRLESymbolSequenceView");
    }

    uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx) const
    {
        //return sequence_->populate(buffer, idx);
        return 0;
    }

    uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx, uint64_t size) const
    {
        //return sequence_->populate(buffer, idx, size);
        return 0;
    }

    virtual uint64_t populate_buffer_while(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol) const
    {
//        return sequence_->populate_while(buffer, idx, symbol);
        return 0;
    }

    virtual uint64_t populate_buffer_entry(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol, bool entry_start) const
    {
        //return sequence_->populate_entry(buffer, idx, symbol, entry_start);
        return 0;
    }


    void configure(const void* ptr)
    {
        sequence_ = ptr_cast<const SeqT>(ptr);
    }

    virtual U8String describe() const {
        return TypeNameFactory<PackedSSRLESymbolSequenceView>::name();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(PackedSSRLESymbolSequenceView);
    }
};

}}
