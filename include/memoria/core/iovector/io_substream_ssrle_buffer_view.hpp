
// Copyright 2021-2022 Victor Smirnov
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

#include <memoria/core/iovector/io_substream_ssrle.hpp>

#include <memoria/core/packed/sseq/packed_ssrle_seq.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/memory/malloc.hpp>

#include <functional>

namespace memoria {
namespace io {

template <size_t AlphabetSize>
class IOSSRLEBufferView final: public IOSSRLEBuffer<AlphabetSize> {

    using Base = IOSSRLEBuffer<AlphabetSize>;

    using typename Base::SeqSizeT;
    using typename Base::SymbolT;
    using typename Base::RunT;
    using typename Base::CodeUnitT;

    using SeqT = PkdSSRLESeqT<AlphabetSize, 256, true>;

    const SeqT* sequence_;

public:
    IOSSRLEBufferView(): sequence_(){}

    IOSSRLEBufferView(IOSSRLEBufferView&&) = delete;
    IOSSRLEBufferView(const IOSSRLEBufferView&) = delete;

    virtual ~IOSSRLEBufferView() noexcept {}

    virtual bool is_indexed() const {
        return true;
    }

    virtual SymbolT alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return true;
    }

    virtual SymbolT symbol(SeqSizeT idx) const {
        return sequence_->access(idx);
    }

    virtual SeqSizeT size() const {
        return sequence_->size();
    }


    virtual void rank_to(SeqSizeT idx, Span<SeqSizeT> values) const
    {
        for (SeqSizeT& vv: values) {
            vv = SeqSizeT{};
        }

        sequence_->ranks(idx, values);
    }

    virtual SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx) const {
        return sequence_->populate_buffer(buffer, idx);
    }

    virtual SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx, SeqSizeT size) const {
        return sequence_->populate_buffer(buffer, idx, size);
    }

    virtual SeqSizeT populate_buffer_while_ge(SymbolsBuffer& buffer, SeqSizeT idx, SymbolT symbol) const {
        return sequence_->populate_buffer_while_ge(buffer, idx, symbol);
    }

    virtual void append(Span<const RunT> runs) {
        MMA_THROW(UnsupportedOperationException());
    }

    virtual void append_run(SymbolT symbol, size_t size) {
        MMA_THROW(UnsupportedOperationException());
    }

    Span<const CodeUnitT> code_units() const {
        return sequence_->symbols();
    }

    std::vector<RunT> symbol_runs(SeqSizeT start = SeqSizeT{}, SeqSizeT size = SeqSizeT::max()) const {
        return sequence_->symbol_runs(start, size);
    }

    virtual void reindex() {
        MEMORIA_MAKE_GENERIC_ERROR("Reindexing is not supported for PackedSSRLESymbolSequenceView").do_throw();
    }

    virtual void reset() {
        MEMORIA_MAKE_GENERIC_ERROR("Resetting is not supported for PackedSSRLESymbolSequenceView").do_throw();
    }

    virtual void dump(std::ostream& out) const {
        DumpStruct(sequence_, out);
    }

    const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }

    void configure(const void* ptr) {
        sequence_ = ptr_cast<const SeqT>(ptr);
    }

    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLEBufferView>::name();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLEBufferView);
    }
};


template <>
class IOSSRLEBufferView<1> final: public IOSSRLEBuffer<1> {

    using Base = IOSSRLEBuffer<1>;

    using typename Base::SeqSizeT;
    using typename Base::SymbolT;
    using typename Base::RunT;
    using typename Base::CodeUnitT;

    size_t size_{};

public:
    IOSSRLEBufferView() {}

    IOSSRLEBufferView(IOSSRLEBufferView&&) = delete;
    IOSSRLEBufferView(const IOSSRLEBufferView&) = delete;

    virtual ~IOSSRLEBufferView() noexcept {}

    virtual bool is_indexed() const {
        return true;
    }

    virtual SymbolT alphabet_size() const {
        return 1;
    }

    virtual bool is_const() const {
        return true;
    }

    virtual SymbolT symbol(SeqSizeT idx) const {
        return 0;
    }

    virtual SeqSizeT size() const {
        return size_;
    }


    virtual void rank_to(SeqSizeT idx, Span<SeqSizeT> values) const
    {
        if (idx <= SeqSizeT{size_})
        {
            values[0] = idx;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range ckeck in IOSSRLEBufferView::append {} {}", idx, size_).do_throw();
        }
    }

    SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx) const
    {
        MEMORIA_V1_ASSERT_TRUE(idx <= size_);
        buffer.append_run(0, size_ - idx);
        return size_;
    }

    SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx, SeqSizeT size) const
    {
        MEMORIA_V1_ASSERT_TRUE(idx + size <= size_)
        buffer.append_run(0, size);
        return idx + size;
    }

    SeqSizeT populate_buffer_while_ge(SymbolsBuffer& buffer, SeqSizeT idx, SeqSizeT symbol) const
    {
        MEMORIA_V1_ASSERT_TRUE(idx <= size_);

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

    virtual void append(Span<const RunT> runs) {
        MMA_THROW(UnsupportedOperationException());
    }

    virtual void append_run(SymbolT symbol, size_t size) {
        MMA_THROW(UnsupportedOperationException());
    }

    Span<const CodeUnitT> code_units() const {
        return Span<const CodeUnitT>{};
    }

    std::vector<RunT> symbol_runs(SeqSizeT start, SeqSizeT size) const
    {
        std::vector<RunT> runs;

        SeqSizeT max = (start + size <= SeqSizeT{size_}) ? size : SeqSizeT{size_} - start;
        runs.push_back(RunT{1, 0, (RunSizeT)max});

        return runs;
    }

    virtual void reindex() {
        MEMORIA_MAKE_GENERIC_ERROR("Reindexing is not supported for PackedSSRLESymbolSequenceView").do_throw();
    }

    virtual void reset() {
        MEMORIA_MAKE_GENERIC_ERROR("Resetting is not supported for PackedSSRLESymbolSequenceView").do_throw();
    }

    virtual void dump(std::ostream& out) const {
        out << "IOSSRLEBufferImpl<1>::size_ = " << size_;
    }

    const std::type_info& sequence_type() const {
        return typeid(IOSSRLEBufferView);
    }

    void configure(const void* ptr) {
        size_ = value_cast<size_t>(ptr);
    }

    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLEBufferView>::name();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLEBufferView);
    }
};


}}
