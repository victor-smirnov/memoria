
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memoria/v1/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>
#include <memoria/v1/core/packed/sseq/small_symbols_buffer.hpp>

namespace memoria {


template <
    Int BitsPerSymbol_,
    template <typename> class ToolsFnType   = BitmapToolsFn
>
struct PackedFSESequenceInputBufferTypes {

    static const Int        BitsPerSymbol           = BitsPerSymbol_;

    template <typename Seq>
    using ToolsFn = ToolsFnType<Seq>;
};



template <typename Types_>
class PkdFSESequenceInputBuffer: public PackedAllocatable {

    using Base = PackedAllocatable;

public:
    static const UInt VERSION = 1;

    using Types  = Types_;
    using MyType = PkdFSESequenceInputBuffer<Types>;

    static const Int BitsPerSymbol = Types::BitsPerSymbol;
    static const Int Indexes                = 1<<BitsPerSymbol;
    static const Int AlphabetSize           = 1<<BitsPerSymbol;

    using Values = core::StaticVector<BigInt, Indexes>;

    using Value = IfThenElse<
                BitsPerSymbol == 8,
                UByte,
                UBigInt
    >;

    using Tools = typename Types::template ToolsFn<MyType>;

    using SymbolsBuffer = SmallSymbolBuffer<BitsPerSymbol>;

    using SizesT = core::StaticVector<Int, 1>;

private:

    Int size_;
    Int max_size_;
    Int alignment_gap_;

    Value buffer_[];

public:
    PkdFSESequenceInputBuffer() {}

    Int& size() {return size_;}
    const Int& size() const {return size_;}

    Int& max_size() {return max_size_;}
    const Int& max_size() const {return max_size_;}

public:
    void init(Int capacity)
    {
        size_ = 0;
        alignment_gap_ = 0;

        max_size_   = capacity;
    }

    void init(SizesT capacity)
    {
        init(capacity[0]);
    }

    void init()
    {
        init(empty_size());
    }

    static constexpr Int block_size(Int elements)
    {
        return sizeof(MyType) + roundUpBitsToAlignmentBlocks(elements * BitsPerSymbol);
    }

    void reset() {
        size_ = 0;
    }

    Int block_size() const {
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

    Int capacity() const {
        return max_size_ - size_;
    }

    Int symbol(Int idx) const {
        return tools().get(symbols(), idx);
    }

    void check() const   {}
    void reindex() const {}

    template <typename Adaptor>
    Int append(Int size, Adaptor&& adaptor)
    {
        auto symbols = this->symbols();

        Int start = size_;
        Int limit = (start + size) <= max_size_ ? size : max_size_ - start;

        auto buf = adaptor(limit);

        tools().move(buf.symbols(), symbols, 0, size_, buf.size());

        size_ += buf.size();

        return buf.size();
    }

    template <typename Adaptor>
    Int append(Adaptor&& adaptor)
    {
        auto symbols = this->symbols();

        Int limit = max_size_ - size_;

        auto buf = adaptor(limit);

        tools().move(buf.symbols(), symbols, 0, size_, buf.size());

        size_ += buf.size();

        return buf.size();
    }


    Int append(const Value* src_symbols, Int start, Int lenght)
    {
        auto symbols = this->symbols();

        Int limit = max_size_ - size_;

        Int limit0 = lenght < limit ? lenght : limit;

        tools().move(src_symbols, symbols, 0, size_, limit0);

        size_ += limit0;

        return limit0;
    }



    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout) const
    {
        out<<"size_       = "<<size_<<endl;
        out<<"max_size_   = "<<max_size_<<endl;
        out<<endl;

        out<<"Data:"<<endl;

        dumpSymbols<Int>(out, size_, BitsPerSymbol, [this](Int pos) -> Int {
            return this->symbol(pos);
        });
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_FSE_INPUT_BUFFER");

        handler->value("PARENT_ALLOCATOR", &(Base::allocator_offset_));

        handler->value("SIZE", &size_);
        handler->value("MAX_SIZE", &max_size_);

        handler->startGroup("DATA", size());

        handler->symbols("SYMBOLS", buffer_, size(), BitsPerSymbol);

        handler->endGroup();

        handler->endGroup();
    }

};


}
