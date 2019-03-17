
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
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

#include <functional>

namespace memoria {
namespace v1 {
namespace io {

template <int32_t AlphabetSize>
class PackedSymbolSequenceNonOwningImpl: public IOSymbolSequence {

    using SeqT = PkdRLESeqT<AlphabetSize>;

    SeqT* sequence_;

public:
    PackedSymbolSequenceNonOwningImpl(SeqT* sequence): sequence_(sequence)
    {
    }

    PackedSymbolSequenceNonOwningImpl(PackedSymbolSequenceNonOwningImpl&&) = delete;
    PackedSymbolSequenceNonOwningImpl(const PackedSymbolSequenceNonOwningImpl&) = delete;

    virtual ~PackedSymbolSequenceNonOwningImpl() noexcept {}

    virtual bool is_indexed() const {
        return true;
    }

    virtual int32_t alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return true;
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
        MMA1_THROW(RuntimeException()) << WhatCInfo("Appending is not supported for PackedSymbolSequenceNonOwningImpl");
    }

    virtual void reindex()
    {
        MMA1_THROW(RuntimeException()) << WhatCInfo("Reindexing is not supported for PackedSymbolSequenceNonOwningImpl");
    }

    virtual void reset() {
        MMA1_THROW(RuntimeException()) << WhatCInfo("Resetting is not supported for PackedSymbolSequenceNonOwningImpl");
    }

    virtual void dump(std::ostream& out) const
    {
        sequence_->dump(out, true);
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }
};



}}}
