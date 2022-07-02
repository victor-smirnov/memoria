
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

#include <memoria/core/packed/sseq/packed_ssrle_seq.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/packed/tools/packed_struct_ptrs.hpp>

#include <functional>

namespace memoria {
namespace io {

template <typename TT>
class IOBufferBase {

protected:
    using ValueT = TT;
    static_assert(std::is_trivially_copyable<ValueT>::value, "IOBufferBase supports only trivially copyable types");

    UniquePtr<ValueT> buffer_;
    size_t capaicty_;
    size_t size_;

protected:
    IOBufferBase(size_t capacity):
        buffer_(capacity > 0 ? allocate_system<ValueT>(capacity) : UniquePtr<ValueT>(nullptr, ::free)),
        capaicty_(capacity),
        size_(0)
    {}

    IOBufferBase(): IOBufferBase(0)
    {}

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capaicty_;
    }

    size_t remaining() const {
        return capaicty_ - size_;
    }

    ValueT& tail() {
        return *buffer_.get();
    }

    ValueT* tail_ptr() {
        return buffer_.get();
    }

    const ValueT& tail() const {
        return *buffer_.get();
    }

    const ValueT* tail_ptr() const {
        return buffer_.get();
    }

    const ValueT* data() const {
        return buffer_.get();
    }

    ValueT* data() {
        return buffer_.get();
    }

    ValueT& head() {
        return *(buffer_.get() + size_ - 1);
    }

    const ValueT& head() const {
        return *(buffer_.get() + size_ - 1);
    }


    void append_value(const ValueT& value)
    {
        ensure(1);
        *(buffer_.get() + size_) = value;
        size_++;
    }

    void append_values(const ValueT* values, size_t size)
    {
        ensure(size);
        MemCpyBuffer(values, buffer_.get() + size_, size);
        size_ += size;
    }

    void append_values(Span<const ValueT> values)
    {
        size_t size = values.size();
        ensure(size);
        MemCpyBuffer(values.data(), buffer_.get() + size_, size);
        size_ += size;
    }

    void ensure(size_t size)
    {
        if (size_ + size > capaicty_)
        {
            enlarge(size);
        }
    }


    void enlarge(size_t requested)
    {
        size_t next_capaicty = capaicty_ * 2;
        if (next_capaicty == 0) next_capaicty = 1;

        while (capaicty_ + requested > next_capaicty)
        {
            next_capaicty *= 2;
        }

        auto new_ptr = allocate_system<ValueT>(next_capaicty);

        if (size_ > 0)
        {
            MemCpyBuffer(buffer_.get(), new_ptr.get(), size_);
        }

        buffer_ = std::move(new_ptr);
        capaicty_ = next_capaicty;
    }

    ValueT& access(size_t idx) {
        return *(buffer_.get() + idx);
    }

    const ValueT& access(size_t idx) const {
        return *(buffer_.get() + idx);
    }

    void clear() {
        size_ = 0;
    }

    void reset()
    {
        size_ = 0;
        capaicty_ = 64;
        buffer_ = allocate_system<ValueT>(capaicty_);
    }

    Span<ValueT> span() {
        return Span<ValueT>(buffer_.get(), size_);
    }

    Span<const ValueT> span() const {
        return Span<ValueT>(buffer_.get(), size_);
    }

    Span<ValueT> span(size_t from) {
        return Span<ValueT>(buffer_.get() + from, size_ - from);
    }

    Span<const ValueT> span(size_t from) const
    {
        return Span<ValueT>(buffer_.get() + from, size_ - from);
    }

    Span<ValueT> span(size_t from, size_t length)
    {
        return Span<ValueT>(buffer_.get() + from, length);
    }

    Span<const ValueT> span(size_t from, size_t length) const
    {
        return Span<ValueT>(buffer_.get() + from, length);
    }
};

template <typename TT>
class DefaultIOBuffer: public IOBufferBase<TT> {
    using Base = IOBufferBase<TT>;

public:
    DefaultIOBuffer(size_t capacity): Base(capacity) {}
    DefaultIOBuffer(): Base() {}

    using Base::append_value;
    using Base::append_values;
    using Base::access;
    using Base::size;
    using Base::clear;
    using Base::head;
    using Base::tail;
    using Base::tail_ptr;
    using Base::data;
    using Base::remaining;
    using Base::reset;
    using Base::span;
    using Base::ensure;

    TT& operator[](size_t idx) {return access(idx);}
    const TT& operator[](size_t idx) const {return access(idx);}

    void emplace_back(const TT& tt) {
        append_value(tt);
    }

    void emplace_back(TT&& tt) {
        append_value(tt);
    }
};


namespace detail {

struct SymbolsRun {
    size_t symbol;
    uint64_t length;
};

}

class SymbolsBuffer: IOBufferBase<detail::SymbolsRun> {
    using Base = IOBufferBase<detail::SymbolsRun>;

    using typename Base::ValueT;
    size_t last_symbol_;
public:
    using Base::clear;
    using Base::reset;
    using Base::size;
    using Base::head;
    using Base::span;

    SymbolsBuffer(size_t symbols):
        SymbolsBuffer(symbols, 64)
    {}

    SymbolsBuffer(size_t symbols, size_t capacity):
        Base(capacity), last_symbol_(symbols - 1)
    {}

    void append_run(size_t symbol, uint64_t length)
    {
        if (MMA_UNLIKELY(size_ == 0))
        {
            append_value(detail::SymbolsRun{symbol, length});
        }
        else if (MMA_LIKELY(head().symbol != symbol))
        {
            auto& hh = head();
            if (hh.symbol < last_symbol_)
            {
                split_head();
            }

            append_value(detail::SymbolsRun{symbol, length});
        }
        else {
            head().length += length;
        }
    }

    detail::SymbolsRun& operator[](size_t idx) {
        return access(idx);
    }

    const detail::SymbolsRun& operator[](size_t idx) const {
        return access(idx);
    }

    void finish()
    {
        if (size_ > 0 && head().symbol < last_symbol_) {
            split_head();
        }
    }

    size_t rank(size_t symbol) const
    {
        size_t sum{};

        for (auto& run: span())
        {
            if (run.symbol == symbol) {
                sum += run.length;
            }
        }

        return sum;
    }

private:
    void split_head()
    {
        auto& element = head();

        if (element.length > 1)
        {
            element.length--;
            append_value(detail::SymbolsRun{element.symbol, 1});
        }
    }
};

struct IOSSRLEBufferBase {

    virtual ~IOSSRLEBufferBase() noexcept = default;

    using SeqSizeT = uint64_t;
    using SymbolT  = size_t;

    virtual bool is_indexed() const                 = 0;
    virtual SymbolT alphabet_size() const           = 0;
    virtual bool is_const() const                   = 0;

    virtual SymbolT symbol(SeqSizeT idx) const      = 0;
    virtual SeqSizeT size() const                   = 0;
    virtual void append_run(SymbolT symbol, size_t size) = 0;

    virtual void reindex()                          = 0;
    virtual void dump(std::ostream& out) const      = 0;

    virtual void rank_to(SeqSizeT idx, Span<SeqSizeT> values) const  = 0;

    virtual SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx) const = 0;
    virtual SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx, SeqSizeT size) const = 0;

    virtual SeqSizeT populate_buffer_while_ge(SymbolsBuffer& buffer, SeqSizeT idx, SymbolT symbol) const = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLEBufferBase>::name();
    }

    virtual const std::type_info& sequence_type() const = 0;
    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLEBufferBase);
    }

    virtual void reset()                            = 0;
    virtual void configure(const void* ptr)         = 0;
};


template <size_t Symbols>
class IOSSRLEBuffer: public IOSSRLEBufferBase {
    using Base = IOSSRLEBufferBase;
public:
    using typename Base::SeqSizeT;
    using typename Base::SymbolT;

    static constexpr SymbolT BITS_PER_SYMBOL = BitsPerSymbolConstexpr(Symbols);

    using RunTraits = SSRLERunTraits<BITS_PER_SYMBOL>;
    using RunT      = SSRLERun<BITS_PER_SYMBOL>;
    using CodeUnitT = typename RunTraits::CodeUnitT;
    using RunSizeT  = typename RunTraits::RunSizeT;

    virtual void append(Span<const RunT> runs) = 0;
    virtual Span<const CodeUnitT> code_units() const = 0;
    virtual std::vector<RunT> symbol_runs(SeqSizeT start, SeqSizeT size) const = 0;
};


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
    using SeqSO = typename SeqT::SparseObject;
    using HolderT = PkdStructHolder<SeqT>;

    std::unique_ptr<HolderT> holder_;
    SeqSO sequence_so_;

    ArenaBuffer<RunT> runs_buf_;


public:
    IOSSRLEBufferImpl()
    {
        size_t size = SeqT::empty_size();
        holder_ = HolderT::make_empty_unique(size);
        sequence_so_ = holder_->get_so();
    }

    IOSSRLEBufferImpl(IOSSRLEBufferImpl&&) = delete;
    IOSSRLEBufferImpl(const IOSSRLEBufferImpl&) = delete;

    virtual ~IOSSRLEBufferImpl() noexcept = default;

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
        assert_indexed();
        return sequence_so_.access(idx);
    }

    virtual SeqSizeT size() const {
        assert_indexed();
        return sequence_so_.size();
    }


    virtual void rank_to(SeqSizeT idx, Span<SeqSizeT> values) const
    {
        assert_indexed();

        for (SeqSizeT& vv: values) {
            vv = SeqSizeT{};
        }

        sequence_so_.ranks(idx, values);
    }

    virtual SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx) const {
        assert_indexed();
        return sequence_so_.populate_buffer(buffer, idx);
    }
    virtual SeqSizeT populate_buffer(SymbolsBuffer& buffer, SeqSizeT idx, SeqSizeT size) const {
        assert_indexed();
        return sequence_so_.populate_buffer(buffer, idx, size);
    }

    virtual SeqSizeT populate_buffer_while_ge(SymbolsBuffer& buffer, SeqSizeT idx, SymbolT symbol) const {
        assert_indexed();
        return sequence_so_.populate_buffer_while_ge(buffer, idx, symbol);
    }

    virtual void check()
    {
        assert_indexed();
        sequence_so_.check();
    }

    virtual void reindex()
    {
        size_t buf_size = runs_buf_.size();
        if (buf_size)
        {
            auto update_state = sequence_so_.make_update_state();
            auto size = sequence_so_.size();
            PkdUpdateStatus status = sequence_so_.prepare_insert(size, update_state.first, runs_buf_.span());
            if (!is_success(status))
            {
                size_t code_units = sequence_so_.get_code_units_num(update_state.first);
                create_empty_sequence(code_units);
            }

            sequence_so_.commit_insert(size, update_state.first, runs_buf_.span());
            runs_buf_.clear();
        }
    }

    virtual void reset() {
        sequence_so_.data()->reset();
        runs_buf_.clear();
    }

    virtual void clear() {
        reset();
    }

    virtual void dump(std::ostream& out) const
    {
        assert_indexed();
        DumpStruct(sequence_so_, out);
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }

    void append(Span<const RunT> runs)
    {
        if (runs_buf_.size()) {
            if (runs.size() == 1) {
                auto res = RunTraits::insert(runs_buf_.head(), runs[0], runs_buf_.head().full_run_length());
                auto span = res.span();
                runs_buf_.head() = span[0];
                for (size_t c = 1; c < span.size(); c++) {
                    runs_buf_.append_value(span[c]);
                }
            }
            else {
                runs_buf_.append_values(runs);
            }
        }
        else {
            runs_buf_.append_values(runs);
        }
    }

    virtual void append_run(SymbolT symbol, size_t size)
    {
        RunT run{1, symbol, size};
        append(Span<const RunT>{&run, 1});
    }

    Span<const CodeUnitT> code_units() const {
        assert_indexed();
        return sequence_so_.data()->symbols();
    }

    std::vector<RunT> symbol_runs(SeqSizeT start, SeqSizeT size) const {
        assert_indexed();
        return sequence_so_.symbol_runs(start, size);
    }


    void configure(const void* ptr) {
        MMA_THROW(UnsupportedOperationException());
    }

private:

    void create_empty_sequence(size_t code_units)
    {
        size_t block_size = SeqT::compute_block_size(code_units * 2);
        holder_ = HolderT::make_empty_unique(block_size);
        sequence_so_ = holder_->get_so();
    }



    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLEBufferImpl>::name();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLEBuffer<AlphabetSize>);
    }

    void assert_indexed() const {
        if (MMA_UNLIKELY(runs_buf_.size())) {
            MEMORIA_MAKE_GENERIC_ERROR("SSRLE Buffer has temporary runs and needs reindexing");
        }
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
