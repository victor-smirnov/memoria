
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_BITMAP_HPP_
#define MEMORIA_CORE_PACKED_FSE_BITMAP_HPP_

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>

namespace memoria {



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



template <typename Types_>
class PackedFSEBitmap: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PackedFSEBitmap<Types>                                              MyType;

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Value                                               Value;

    static const Int BitsPerSymbol                                              = Types::BitsPerSymbol;

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

    static Int block_size(Int elements)
    {
        return sizeof(MyType) + roundUpBitsToAlignmentBlocks(elements * BitsPerSymbol);
    }

    Int block_size() const {
        const Allocator* alloc = this->allocator();
        return alloc->element_size(this);
    }

    static int empty_size() {
        return sizeof(MyType);
    }

    static Int block_size_bs(Int block_size)
    {
        return sizeof(MyType) + block_size;
    }

    BitmapAccessor<Value*, Value, BitsPerSymbol>
    value(Int idx) {
        return BitmapAccessor<Value*, Value, BitsPerSymbol>(buffer_, idx);
    }

    BitmapAccessor<const Value*, Value, BitsPerSymbol>
    value(Int idx) const {
        return BitmapAccessor<const Value*, Value, BitsPerSymbol>(buffer_, idx);
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

    Int capacity() const {
        return max_size_ - size_;
    }

    void enlarge(Int elements)
    {
        Allocator* alloc = Base::allocator();
        Int amount = roundUpBitToBytes(roundUpBitToBytes(elements * BitsPerSymbol));
        Int size = alloc->element_size(this);
        Int new_size = alloc->resizeBlock(this, size + amount);
        max_size_ = (new_size - empty_size()) * 8 / BitsPerSymbol;
    }

    bool insertSpace(Int idx, Int space)
    {
        if (space > capacity())
        {
            enlarge(space - capacity());
        }

        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, size_);
        MEMORIA_ASSERT(size_ + space, <=, max_size_);

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

    void removeSpace(Int idx, Int space)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, size_);
        MEMORIA_ASSERT(idx + space, <=, size_);

        Value* data = this->data();

        Int remainder = (size_ - idx - space) * BitsPerSymbol;
        MoveBits(data, data, (idx + space) * BitsPerSymbol, idx * BitsPerSymbol, remainder);

        size_ -= space;
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
};


}


#endif
