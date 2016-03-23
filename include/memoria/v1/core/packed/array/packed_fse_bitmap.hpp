
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memoria/v1/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>

namespace memoria {
namespace v1 {



template <
    Int BitsPerSymbol_,
    typename V = UBigInt,
    typename Allocator_ = PackedAllocator
>
struct PackedFSEBitmapTypes {

    static const Int        BitsPerSymbol           = BitsPerSymbol_;

    typedef V               Value;
    typedef Allocator_      Allocator;
};

template <typename Types_> class PackedFSEBitmap;

template <Int BitsPerSymbol>
using PackedFSEBitmapT = PackedFSEBitmap<PackedFSEBitmapTypes<BitsPerSymbol>>;

template <typename Types_>
class PackedFSEBitmap: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION                                                   = 1;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

    typedef Types_                                                              Types;
    typedef PackedFSEBitmap<Types>                                              MyType;

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Value                                               Value;

    static const Int BitsPerSymbol                                              = Types::BitsPerSymbol;

    typedef BitmapAccessor<Value*, Value, BitsPerSymbol>                        SymbolAccessor;
    typedef BitmapAccessor<const Value*, Value, BitsPerSymbol>                  ConstSymbolAccessor;

private:

    Int size_;
    Int max_size_;
    Int alignment_gap_;

    Value buffer_[];

public:
    PackedFSEBitmap() {}

    Int& size() {return size_;}
    const Int& size() const {return size_;}

    Int& max_size() {return max_size_;}
    const Int& max_size() const {return max_size_;}

public:
    void init(Int block_size)
    {
        size_ = 0;
        alignment_gap_ = 0;

        Int data_size = block_size - empty_size();

        max_size_   = data_size * 8 / BitsPerSymbol;
    }

    void init()
    {
        init(empty_size());
    }

    static constexpr Int packed_block_size(Int elements)
    {
        return block_size(elements);
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

    SymbolAccessor value(Int idx) {
        return SymbolAccessor(buffer_, idx);
    }

    ConstSymbolAccessor value(Int idx) const {
        return ConstSymbolAccessor(buffer_, idx);
    }

    SymbolAccessor symbol(Int idx) {
        return SymbolAccessor(buffer_, idx);
    }

    ConstSymbolAccessor symbol(Int idx) const {
        return ConstSymbolAccessor(buffer_, idx);
    }


    template <typename T, Int I>
    void sums(Int from, Int to, StaticVector<T, I>& values) const
    {
        values[0] += to - from;
    }

    template <typename T, Int I>
    void sums(StaticVector<T, I>& values) const
    {
        values[0] += size();
    }

    Value* data() {
        return buffer_;
    }

    const Value* data() const {
        return buffer_;
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

    void enlarge1(Int elements)
    {
        Allocator* alloc = Base::allocator();
        Int amount = roundUpBitToBytes(roundUpBitToBytes(elements * BitsPerSymbol));
        Int size = alloc->element_size(this);
        Int new_size = alloc->resizeBlock(this, size + amount + empty_size());
        max_size_ = (new_size - empty_size()) * 8 / BitsPerSymbol;
    }




    bool insertSpace(Int idx, Int space)
    {
        if (space > capacity())
        {
            enlarge(space - capacity());
        }

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <=, size_);
        MEMORIA_V1_ASSERT(size_ + space, <=, max_size_);

        Int remainder = (size_ - idx) * BitsPerSymbol;

        Value* data = this->data();

        MoveBits(data, data, idx * BitsPerSymbol, (idx + space) * BitsPerSymbol, remainder);

        size_ += space;

        return true;
    }

    bool insert(Int idx, Value value)
    {
        if (insertSpace(idx, 1))
        {
            this->value(idx) = value;
            return true;
        }
        return false;
    }

    void remove(Int start, Int end) {
        removeSpace(start, end);
    }

    void removeSpace(Int start, Int end)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, size_);
        MEMORIA_V1_ASSERT(end, <=, size_);

        Value* data = this->data();

        Int remainder = (size_ - end) * BitsPerSymbol;
        MoveBits(data, data, end * BitsPerSymbol, start * BitsPerSymbol, remainder);

        shrink(end - start);

        size_ -= (end - start);
    }

    void check() const {}

    // ==================================== Node =========================================== //

    void splitTo(MyType* other, Int idx)
    {
        Int to_move     = this->size() - idx;
        Int other_size  = other->size();

        other->enlargeData(to_move);

        move(other->symbols(), other->symbols(), 0, to_move, other_size);

        move(this->symbols(), other->symbols(), idx, 0, to_move);

        other->size() += to_move;

        removeSpace(idx, this->size());
    }

    void mergeWith(MyType* other) const
    {
        Int my_size     = this->size();
        Int other_size  = other->size();

        other->enlargeData(my_size);

        move(this->symbols(), other->symbols(), 0, other_size, my_size);

        other->size() += my_size;
    }

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

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_FSE_BITMAP");

        handler->value("PARENT_ALLOCATOR", &(Base::allocator_offset_));

        handler->value("SIZE", &size_);
        handler->value("MAX_SIZE", &max_size_);

        handler->startGroup("DATA", size());

        handler->symbols("SYMBOLS", buffer_, size(), BitsPerSymbol);

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        FieldFactory<Int>::serialize(buf, size_);
        FieldFactory<Int>::serialize(buf, max_size_);

        FieldFactory<Value>::serialize(buf, buffer_, symbols_buffer_size());
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        FieldFactory<Int>::deserialize(buf, size_);
        FieldFactory<Int>::deserialize(buf, max_size_);

        FieldFactory<Value>::deserialize(buf, buffer_, symbols_buffer_size());
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

    void enlargeData(Int elements) {
        enlarge(elements);
    }

    void enlarge(Int elements)
    {
        Allocator* alloc = Base::allocator();
        Int amount = roundUpBitsToAlignmentBlocks((size_ + elements) * BitsPerSymbol);

        Int new_size = alloc->resizeBlock(this, amount + empty_size());
        max_size_ = (new_size - empty_size()) * 8 / BitsPerSymbol;
    }

    void shrink(Int elements)
    {
        MEMORIA_V1_ASSERT(size_, >=, elements);

        Allocator* alloc = Base::allocator();
        Int amount = roundUpBitsToAlignmentBlocks((size_ - elements) * BitsPerSymbol);

        Int new_size = alloc->resizeBlock(this, amount + empty_size());
        max_size_ = (new_size - empty_size()) * 8 / BitsPerSymbol;
    }
};


}}