
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_SEQUENCE_INPUT_BUFFER_HPP_
#define MEMORIA_CORE_PACKED_FSE_SEQUENCE_INPUT_BUFFER_HPP_

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>

namespace memoria {



template <
    Int BitsPerSymbol_,
    typename V = UBigInt
>
struct PackedFSESequenceInputBufferTypes {
    static const Int        BitsPerSymbol           = BitsPerSymbol_;
    typedef V               Value;
};



template <typename Types_>
class PkdFSESequenceInputBuffer: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PackedFSEBitmap<Types>                                              MyType;

    typedef PackedAllocator                                           			Allocator;
    typedef typename Types::Value                                               Value;

    static const Int BitsPerSymbol                                              = Types::BitsPerSymbol;


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

    void init()
    {
        init(empty_size());
    }

    static constexpr Int block_size(Int elements)
    {
        return sizeof(MyType) + roundUpBitsToAlignmentBlocks(elements * BitsPerSymbol);
    }

    Int block_size() const {
        const Allocator* alloc = this->allocator();
        return alloc->element_size(this);
    }

    static constexpr int empty_size() {
        return sizeof(MyType);
    }

    static constexpr Int block_size_bs(Int block_size)
    {
        return sizeof(MyType) + block_size;
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


    bool insertSpace(Int idx, Int space)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, size_);
        MEMORIA_ASSERT(size_ + space, <=, max_size_);

        Int remainder = (size_ - idx) * BitsPerSymbol;

        Value* data = this->data();

        MoveBits(data, data, idx * BitsPerSymbol, (idx + space) * BitsPerSymbol, remainder);

        size_ += space;

        return true;
    }

    void insert(Int idx, Value value)
    {
        insertSpace(idx, 1);
        this->value(idx) = value;
    }

    void remove(Int start, Int end) {
        removeSpace(start, end);
    }

    void removeSpace(Int start, Int end)
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, size_);
        MEMORIA_ASSERT(end, <=, size_);

        Value* data = this->data();

        Int remainder = (size_ - end) * BitsPerSymbol;
        MoveBits(data, data, end * BitsPerSymbol, start * BitsPerSymbol, remainder);

        size_ -= (end - start);
    }

    void check() const {}





    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout) const
    {
        out<<"size_       = "<<size_<<endl;
        out<<"max_size_   = "<<max_size_<<endl;
        out<<endl;

        out<<"Data:"<<endl;

        dumpSymbols<Value>(out, size_, BitsPerSymbol, [this](Int pos) -> Value {
            return this->value(pos);
        });
    }


private:
    Int symbols_buffer_size() const
    {
        Int block_size  = this->block_size();
        Int buffer_size = block_size - sizeof(MyType);

        Int bit_size    = buffer_size * 8;
        Int byte_size   = Base::roundUpBitsToAlignmentBlocks(bit_size);

        return byte_size / sizeof(Value);
    }

    void move(Value* symbols, Int from, Int to, Int lenght) const
    {
        MoveBits(symbols, symbols, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }

    void move(const Value* src, Value* dst, Int from, Int to, Int lenght) const
    {
        MoveBits(src, dst, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }
};


}


#endif
