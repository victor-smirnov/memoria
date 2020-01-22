
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

#include <functional>

namespace memoria {
namespace io {

template <int32_t AlphabetSize>
class PackedRLESymbolSequenceView;

template <>
class PackedRLESymbolSequenceView<1>: public IOSymbolSequence {

    uint64_t size_{};

public:
    PackedRLESymbolSequenceView()
    {
    }

    PackedRLESymbolSequenceView(PackedRLESymbolSequenceView&&) = delete;
    PackedRLESymbolSequenceView(const PackedRLESymbolSequenceView&) = delete;

    virtual ~PackedRLESymbolSequenceView() noexcept {}

    virtual bool is_indexed() const {
        return true;
    }

    virtual int32_t alphabet_size() const {
        return 1;
    }

    virtual bool is_const() const {
        return true;
    }

    virtual int32_t symbol(uint64_t idx) const {
        return 0;
    }

    virtual uint64_t size() const {
        return size_;
    }

    virtual const void* buffer() const {
        return &size_;
    }

    virtual void* buffer() {
        MMA_THROW(UnsupportedOperationException());
    }

    virtual void rank_to(uint64_t idx, uint64_t* values) const
    {
        values[0] = idx;
    }

    virtual void append(int32_t symbol, uint64_t length)
    {
        MMA_THROW(RuntimeException()) << WhatCInfo("Appending is not supported for PackedSymbolSequenceNonOwningImpl");
    }

    virtual void reindex()
    {
        MMA_THROW(RuntimeException()) << WhatCInfo("Reindexing is not supported for PackedSymbolSequenceNonOwningImpl");
    }

    virtual void reset() {
        MMA_THROW(RuntimeException()) << WhatCInfo("Resetting is not supported for PackedSymbolSequenceNonOwningImpl");
    }

    virtual void dump(std::ostream& out) const
    {

    }

    virtual const std::type_info& sequence_type() const {
        return typeid(PackedRLESymbolSequenceView<1>);
    }

    virtual void copy_to(IOSubstream& target, int32_t start, int32_t length) const
    {
        IOSymbolSequence& target_stream = substream_cast<IOSymbolSequence>(target);
        target_stream.append_from(*this, start, length);
    }

    void append_from(const IOSymbolSequence& source, int32_t start, int32_t length) {
        MMA_THROW(RuntimeException())
                << WhatCInfo("Appending to not supported for PackedSymbolSequenceNonOwningImpl");
    }

    uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx) const
    {
        MEMORIA_V1_ASSERT_TRUE(idx >= 0 && idx <= size_);
        buffer.append_run(0, size_ - idx);
        return size_;
    }

    uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx, uint64_t size) const
    {
        MEMORIA_V1_ASSERT_TRUE(idx >= 0 && idx <= size_);
        MEMORIA_V1_ASSERT_TRUE(idx + size >= 0 && idx + size <= size_)
        buffer.append_run(0, size);
        return idx + size;
    }

    virtual uint64_t populate_buffer_while(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol) const
    {
        MEMORIA_V1_ASSERT_TRUE(idx >= 0 && idx <= size_);

        if (symbol == 0)
        {
            if (idx < size_)
            {
                buffer.append_run(0, size_ - idx);
            }

            return size_;
        }
        else {
            return idx;
        }
    }

    virtual uint64_t populate_buffer_entry(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol, bool entry_start) const
    {
        MMA_THROW(UnsupportedOperationException());
    }

    virtual void configure(const void* ptr)
    {
        size_ = T2T<uint64_t>(ptr);
    }
};


}}
