
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
class IOSSRLEBufferImpl: public IOSSRLEBuffer<AlphabetSize> {

    using Base = IOSSRLEBuffer<AlphabetSize>;

    using typename Base::SeqSizeT;
    using typename Base::SymbolT;
    using typename Base::RunT;
    using typename Base::RunTraits;
    using typename Base::CodeUnitT;

    static_assert (AlphabetSize >= 2 && AlphabetSize <= 256, "");
    static constexpr size_t Bps = BitsPerSymbolConstexpr(AlphabetSize);

    using SeqT = PkdSSRLESeqT<AlphabetSize, 256, true>;

    SeqT* sequence_;

public:
    IOSSRLEBufferImpl()
    {
        size_t size = SeqT::empty_size();
        sequence_ = ptr_cast<SeqT>(allocate_system<uint8_t>(size).release());
        sequence_->allocatable().setTopLevelAllocator();
        (void)sequence_->init();
    }

    IOSSRLEBufferImpl(IOSSRLEBufferImpl&&) = delete;
    IOSSRLEBufferImpl(const IOSSRLEBufferImpl&) = delete;

    virtual ~IOSSRLEBufferImpl() noexcept
    {
        free_system(sequence_);
    }

    virtual bool is_indexed() const {
        return true;
    }

    virtual SymbolT alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return false;
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

    virtual void check()
    {
        sequence_->check();
    }

    virtual void reindex()
    {
        while (is_packed_error(sequence_->reindex())) {
            enlarge();
        }
    }

    virtual void reset() {
        sequence_->reset();
    }

    virtual void dump(std::ostream& out) const
    {
        DumpStruct(sequence_, out);
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }

    void append(Span<const RunT> runs) {
        while (true) {
            size_t required = sequence_->append(runs).get_or_throw();
            if (required > 0) {
                enlarge(required);
            }
            else {
                break;
            }
        }
    }

    virtual void append_run(SymbolT symbol, size_t size)
    {
        RunT run{1, symbol, size};
        append(Span<const RunT>{&run, 1});
    }

    Span<const CodeUnitT> code_units() const {
        return sequence_->symbols();
    }

    std::vector<RunT> symbol_runs(SeqSizeT start, SeqSizeT size) const {
        return sequence_->symbol_runs(start, size);
    }


    void configure(const void* ptr) {
        MMA_THROW(UnsupportedOperationException());
    }

private:

    template <typename T>
    bool is_packed_error(Result<T>&& res)
    {
        if (res.is_error()) {
            if (res.is_packed_error()) {
                return true;
            }
            else {
                res.get_or_throw();
            }
        }

        return false;
    }

    void enlarge()
    {
        int32_t bs = sequence_->block_size();
        SeqT* new_seq = ptr_cast<SeqT>(allocate_system<uint8_t>(bs * 2).release());
        memcpy(new_seq, sequence_, bs);
        new_seq->set_block_size(bs * 2);

        free_system(sequence_);

        sequence_ = new_seq;
    }

    void enlarge(size_t syms_block_size)
    {
        size_t current_bs = sequence_->block_size();
        size_t bs = SeqT::block_size(syms_block_size);
        SeqT* new_seq = ptr_cast<SeqT>(allocate_system<uint8_t>(bs).release());
        memcpy(new_seq, sequence_, current_bs);
        new_seq->set_block_size(bs);
        new_seq->resize_block(SeqT::SYMBOLS, syms_block_size).get_or_throw();

        free_system(sequence_);

        sequence_ = new_seq;
    }


    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLEBufferImpl>::name();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLEBuffer<AlphabetSize>);
    }
};


template <>
class IOSSRLEBufferImpl<1>: public IOSSRLEBuffer<1> {

    using Base = IOSSRLEBuffer<1>;

    using typename Base::SeqSizeT;
    using typename Base::RunSizeT;
    using typename Base::SymbolT;
    using typename Base::RunT;
    using typename Base::RunTraits;
    using typename Base::CodeUnitT;

    static constexpr size_t AlphabetSize = 1;
    static constexpr size_t Bps = 1;

    size_t size_{0};


public:
    IOSSRLEBufferImpl(){}

    IOSSRLEBufferImpl(IOSSRLEBufferImpl&&) = delete;
    IOSSRLEBufferImpl(const IOSSRLEBufferImpl&) = delete;

    virtual ~IOSSRLEBufferImpl() noexcept {}

    virtual bool is_indexed() const {
        return true;
    }

    virtual SymbolT alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return false;
    }

    virtual SymbolT symbol(SeqSizeT idx) const {
        return 0;
    }

    virtual SeqSizeT size() const {
        return size_;
    }


    virtual void rank_to(SeqSizeT idx, Span<SeqSizeT> values) const
    {
        values[0] = SeqSizeT{idx};
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

    virtual void check() {}

    virtual void reindex() {}

    virtual void reset() {
        size_ = 0;
    }

    virtual void dump(std::ostream& out) const
    {
        out << "IOSSRLEBufferImpl<1>::size_ = " << size_;
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(IOSSRLEBufferImpl);
    }

    void append(Span<const RunT> runs)
    {
        for (const RunT& run: runs){
            size_ += run.run_length();
        }
    }

    virtual void append_run(SymbolT symbol, size_t size) {
        size_ += size;
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


    void configure(const void* ptr) {
        MMA_THROW(UnsupportedOperationException());
    }

private:

    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLEBufferImpl>::name();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLEBuffer<AlphabetSize>);
    }
};


}}
