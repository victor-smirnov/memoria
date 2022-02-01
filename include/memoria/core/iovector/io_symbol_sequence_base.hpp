
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

#include <memoria/core/iovector/io_substream_base.hpp>

#include <memoria/core/tools/type_name.hpp>

#include <memoria/core/iovector/io_buffer_base.hpp>

#include <functional>
#include <tuple>

namespace memoria {
namespace io {

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

struct IOSymbolSequence: IOSubstream {

    virtual bool is_indexed() const                 = 0;
    virtual int32_t alphabet_size() const           = 0;
    virtual bool is_const() const                   = 0;


    virtual int32_t symbol(uint64_t idx) const      = 0;
    virtual uint64_t size() const                   = 0;
    virtual const void* buffer() const              = 0;
    virtual void* buffer()                          = 0;

    virtual void reindex()                          = 0;
    virtual void dump(std::ostream& out) const      = 0;

    virtual void rank_to(uint64_t idx, uint64_t* values) const  = 0;
    virtual void append(int32_t symbol, uint64_t length)        = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IOSymbolSequence>::name();
    }

    virtual const std::type_info& sequence_type() const = 0;
    virtual const std::type_info& substream_type() const {
        return typeid(IOSymbolSequence);
    }

    virtual void reset()                            = 0;
    virtual void configure(const void* ptr)         = 0;

    virtual uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx) const = 0;
    virtual uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx, uint64_t size) const = 0;

    virtual uint64_t populate_buffer_while(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol) const = 0;
    virtual uint64_t populate_buffer_entry(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol, bool entry_start) const = 0;

    virtual void append_from(const IOSymbolSequence& source, int32_t start, int32_t length) = 0;
};



}}
