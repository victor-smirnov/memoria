
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

#include <memoria/v1/core/iovector/io_substream_base.hpp>

#include <memoria/v1/core/tools/type_name.hpp>

#include <memoria/v1/core/iovector/io_buffer_base.hpp>

#include <functional>
#include <tuple>

namespace memoria {
namespace v1 {
namespace io {

namespace _ {

struct SymbolsRun {
    int32_t symbol;
    uint64_t length;
};

}

class SymbolsBuffer: IOBufferBase<_::SymbolsRun> {
    using Base = IOBufferBase<_::SymbolsRun>;

    using typename Base::ValueT;
    int32_t last_symbol_;
public:
    using Base::clear;
    using Base::reset;
    using Base::size;
    using Base::head;
    using Base::span;

    SymbolsBuffer(int32_t symbols):
        SymbolsBuffer(symbols, 64)
    {}

    SymbolsBuffer(int32_t symbols, size_t capacity):
        Base(capacity), last_symbol_(symbols - 1)
    {}

    void append_run(int32_t symbol, uint64_t length)
    {
        if (MMA1_UNLIKELY(size_ == 0))
        {
            append_value(_::SymbolsRun{symbol, length});
        }
        else if (MMA1_LIKELY(head().symbol != symbol))
        {
            auto& hh = head();
            if (hh.symbol < last_symbol_)
            {
                split_head();
            }

            append_value(_::SymbolsRun{symbol, length});
        }
        else {
            head().length += length;
        }
    }

    _::SymbolsRun& operator[](size_t idx) {
        return access(idx);
    }

    const _::SymbolsRun& operator[](size_t idx) const {
        return access(idx);
    }

    void finish()
    {
        if (size_ > 0 && head().symbol < last_symbol_) {
            split_head();
        }
    }

    size_t rank(int32_t symbol) const
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
            append_value(_::SymbolsRun{element.symbol, 1});
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
        return TypeNameFactory<IOSymbolSequence>::name().to_u8();
    }

    virtual const std::type_info& sequence_type() const         = 0;
    virtual const std::type_info& substream_type() const {
        return typeid(IOSymbolSequence);
    }

    virtual void reset()                            = 0;
    virtual void configure(void* ptr)               = 0;

    virtual uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx) const = 0;
    virtual uint64_t populate_buffer(SymbolsBuffer& buffer, uint64_t idx, uint64_t size) const = 0;

    virtual uint64_t populate_buffer_while(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol) const = 0;
    virtual uint64_t populate_buffer_entry(SymbolsBuffer& buffer, uint64_t idx, int32_t symbol, bool entry_start) const = 0;

    virtual void append_from(const IOSymbolSequence& source, int32_t start, int32_t length) = 0;
};



}}}
