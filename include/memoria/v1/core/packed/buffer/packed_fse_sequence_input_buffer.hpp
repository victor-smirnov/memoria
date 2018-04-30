
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memoria/v1/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>
#include <memoria/v1/core/packed/sseq/small_symbols_buffer.hpp>

namespace memoria {
namespace v1 {


template <
    int32_t BitsPerSymbol_,
    template <typename> class ToolsFnType   = BitmapToolsFn
>
struct PackedFSESequenceInputBufferTypes {

    static const int32_t        BitsPerSymbol           = BitsPerSymbol_;

    template <typename Seq>
    using ToolsFn = ToolsFnType<Seq>;
};



template <typename Types_>
class PkdFSESequenceInputBuffer: public PackedAllocatable {

    using Base = PackedAllocatable;

public:
    static const uint32_t VERSION = 1;

    using Types  = Types_;
    using MyType = PkdFSESequenceInputBuffer<Types>;

    static const int32_t BitsPerSymbol = Types::BitsPerSymbol;
    static const int32_t Indexes                = 1<<BitsPerSymbol;
    static const int32_t AlphabetSize           = 1<<BitsPerSymbol;

    using Values = core::StaticVector<int64_t, Indexes>;

    using Value = IfThenElse<
                BitsPerSymbol == 8,
                uint8_t,
                uint64_t
    >;

    using Tools = typename Types::template ToolsFn<MyType>;

    using SymbolsBuffer = SmallSymbolBuffer<BitsPerSymbol>;

    using SizesT = core::StaticVector<int32_t, 1>;

    class AppendState {
        int32_t size_;
    public:
        AppendState(): size_(0) {}
        AppendState(int32_t size): size_(size) {}

        int32_t& size() {return size_;}
        const int32_t& size() const {return size_;}
    };

private:

    int32_t size_;
    int32_t max_size_;
    int32_t alignment_gap_;

    Value buffer_[];

public:
    PkdFSESequenceInputBuffer() {}

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    int32_t& max_size() {return max_size_;}
    const int32_t& max_size() const {return max_size_;}

public:
    OpStatus init(int32_t capacity)
    {
        size_ = 0;
        alignment_gap_ = 0;

        max_size_   = capacity;

        return OpStatus::OK;
    }

    OpStatus init(SizesT capacity)
    {
        return init(capacity[0]);
    }

    OpStatus init()
    {
        return init(empty_size());
    }

    static constexpr int32_t block_size(int32_t elements)
    {
        return sizeof(MyType) + roundUpBitsToAlignmentBlocks(elements * BitsPerSymbol);
    }

    static constexpr int32_t block_size(const SizesT& elements)
    {
        return sizeof(MyType) + roundUpBitsToAlignmentBlocks(elements[0] * BitsPerSymbol);
    }

    OpStatus reset() {
        size_ = 0;
        return OpStatus::OK;
    }

    int32_t block_size() const {
        const PackedAllocator* alloc = this->allocator();
        return alloc->element_size(this);
    }

    static constexpr int empty_size() {
        return sizeof(MyType);
    }

    Tools tools() const {
        return Tools(*this);
    }

    Value* symbols() {
        return buffer_;
    }

    const Value* symbols() const {
        return buffer_;
    }

    int32_t capacity() const {
        return max_size_ - size_;
    }

    int32_t symbol(int32_t idx) const {
        return tools().get(symbols(), idx);
    }

    void check() const   {}
    void reindex() const {}

    template <typename Adaptor>
    int32_t append(int32_t size, Adaptor&& adaptor)
    {
        auto symbols = this->symbols();

        int32_t start = size_;
        int32_t limit = (start + size) <= max_size_ ? size : max_size_ - start;

        auto buf = adaptor(limit);

        tools().move(buf.symbols(), symbols, 0, size_, buf.size());

        size_ += buf.size();

        return buf.size();
    }

    template <typename Adaptor>
    int32_t append(Adaptor&& adaptor)
    {
        auto symbols = this->symbols();

        int32_t limit = max_size_ - size_;

        auto buf = adaptor(limit);

        tools().move(buf.symbols(), symbols, 0, size_, buf.size());

        size_ += buf.size();

        return buf.size();
    }


    int32_t append(const Value* src_symbols, int32_t start, int32_t lenght)
    {
        auto symbols = this->symbols();

        int32_t limit = max_size_ - size_;

        int32_t limit0 = lenght < limit ? lenght : limit;

        tools().move(src_symbols, symbols, start, size_, limit0);

        size_ += limit0;

        return limit0;
    }



    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = std::cout) const
    {
        out << "size_       = " << size_ << std::endl;
        out << "max_size_   = " << max_size_ << std::endl;
        out << std::endl;

        out << "Data:" << std::endl;

        dumpSymbols<int32_t>(out, size_, BitsPerSymbol, [this](int32_t pos) -> int32_t {
            return this->symbol(pos);
        });
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_FSE_INPUT_BUFFER");

        handler->value("PARENT_ALLOCATOR", &this->allocator_offset_);

        handler->value("SIZE", &size_);
        handler->value("MAX_SIZE", &max_size_);

        handler->startGroup("DATA", size());

        handler->symbols("SYMBOLS", buffer_, size(), BitsPerSymbol);

        handler->endGroup();

        handler->endGroup();
    }

};


}}
