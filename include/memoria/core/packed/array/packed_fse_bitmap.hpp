
// Copyright 2013 Victor Smirnov
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

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>

namespace memoria {

template <
    int32_t BitsPerSymbol_,
    typename V = uint64_t,
    typename Allocator_ = PackedAllocator
>
struct PackedFSEBitmapTypes {

    static const int32_t        BitsPerSymbol           = BitsPerSymbol_;

    typedef V               Value;
    typedef Allocator_      Allocator;
};

template <typename Types_> class PackedFSEBitmap;

template <int32_t BitsPerSymbol>
using PackedFSEBitmapT = PackedFSEBitmap<PackedFSEBitmapTypes<BitsPerSymbol>>;

template <typename Types_>
class PackedFSEBitmap {

public:
    static const uint32_t VERSION                                                   = 1;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

    typedef Types_                                                              Types;
    typedef PackedFSEBitmap<Types>                                              MyType;

    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Value                                               Value;

    static const int32_t BitsPerSymbol                                          = Types::BitsPerSymbol;

    typedef BitmapAccessor<Value*, Value, BitsPerSymbol>                        SymbolAccessor;
    typedef BitmapAccessor<const Value*, Value, BitsPerSymbol>                  ConstSymbolAccessor;

    using SizesT = core::StaticVector<int32_t, 1>;
    using ReadState = SizesT;

private:
    PackedAllocatable header_;

    int32_t size_;
    int32_t max_size_;
    int32_t alignment_gap_;

    Value buffer_[];

public:
    PackedFSEBitmap() = default;

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    int32_t& max_size() {return max_size_;}
    const int32_t& max_size() const {return max_size_;}

public:
    VoidResult init(int32_t block_size) noexcept
    {
        size_ = 0;
        alignment_gap_ = 0;

        int32_t data_size = block_size - empty_size();

        max_size_   = data_size * 8 / BitsPerSymbol;

        return VoidResult::of();
    }

    VoidResult init() noexcept
    {
        return init(empty_size());
    }

    static constexpr int32_t packed_block_size(int32_t elements)
    {
        return block_size(elements);
    }

    static constexpr int32_t block_size(int32_t elements)
    {
        return sizeof(MyType) + PackedAllocatable::roundUpBitsToAlignmentBlocks(elements * BitsPerSymbol);
    }

    int32_t block_size() const {
        const Allocator* alloc = header_.allocator();
        return alloc->element_size(this);
    }

    static constexpr int empty_size() {
        return sizeof(MyType);
    }

    static constexpr int32_t block_size_bs(int32_t block_size)
    {
        return sizeof(MyType) + block_size;
    }

    SymbolAccessor value(int32_t idx) {
        return SymbolAccessor(buffer_, idx);
    }

    ConstSymbolAccessor value(int32_t idx) const {
        return ConstSymbolAccessor(buffer_, idx);
    }

    SymbolAccessor symbol(int32_t idx) {
        return SymbolAccessor(buffer_, idx);
    }

    ConstSymbolAccessor symbol(int32_t idx) const {
        return ConstSymbolAccessor(buffer_, idx);
    }


    template <typename T, int32_t I>
    void sums(int32_t from, int32_t to, StaticVector<T, I>& values) const
    {
        values[0] += to - from;
    }

    template <typename T, int32_t I>
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

    int32_t capacity() const {
        return max_size_ - size_;
    }

    void enlarge1(int32_t elements)
    {
        Allocator* alloc = header_.allocator();
        int32_t amount = PackedAllocatable::roundUpBitToBytes(PackedAllocatable::roundUpBitToBytes(elements * BitsPerSymbol));
        int32_t size = alloc->element_size(this);
        int32_t new_size = alloc->resizeBlock(this, size + amount + empty_size());
        max_size_ = (new_size - empty_size()) * 8 / BitsPerSymbol;
    }




    bool insertSpace(int32_t idx, int32_t space)
    {
        if (space > capacity())
        {
            enlarge(space - capacity());
        }

        MEMORIA_V1_ASSERT(idx, >=, 0);
        MEMORIA_V1_ASSERT(idx, <=, size_);
        MEMORIA_V1_ASSERT(size_ + space, <=, max_size_);

        int32_t remainder = (size_ - idx) * BitsPerSymbol;

        Value* data = this->data();

        MoveBits(data, data, idx * BitsPerSymbol, (idx + space) * BitsPerSymbol, remainder);

        size_ += space;

        return true;
    }

    bool insert(int32_t idx, Value value)
    {
        if (insertSpace(idx, 1))
        {
            this->value(idx) = value;
            return true;
        }
        return false;
    }

    void remove(int32_t start, int32_t end) {
        removeSpace(start, end);
    }

    void removeSpace(int32_t start, int32_t end)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, size_);
        MEMORIA_V1_ASSERT(end, <=, size_);

        Value* data = this->data();

        int32_t remainder = (size_ - end) * BitsPerSymbol;
        MoveBits(data, data, end * BitsPerSymbol, start * BitsPerSymbol, remainder);

        shrink(end - start);

        size_ -= (end - start);
    }

    void check() const {}

    ReadState positions(int32_t idx) const {
        return ReadState(idx);
    }



    // ==================================== Node =========================================== //

    VoidResult splitTo(MyType* other, int32_t idx) noexcept
    {
        int32_t to_move     = this->size() - idx;
        int32_t other_size  = other->size();

        MEMORIA_TRY_VOID(other->enlargeData(to_move));

        move(other->symbols(), other->symbols(), 0, to_move, other_size);

        move(this->symbols(), other->symbols(), idx, 0, to_move);

        other->size() += to_move;

        return removeSpace(idx, this->size());
    }

    VoidResult mergeWith(MyType* other) const noexcept
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        MEMORIA_TRY_VOID(other->enlargeData(my_size));

        move(this->symbols(), other->symbols(), 0, other_size, my_size);

        other->size() += my_size;

        return VoidResult::of();
    }

    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = std::cout) const
    {
        out << "size_       = " << size_ << std::endl;
        out << "max_size_   = " << max_size_ << std::endl;
        out << std::endl;

        out << "Data:" << std::endl;

        dumpSymbols<Value>(out, size_, BitsPerSymbol, [this](int32_t pos) -> Value {
            return this->value(pos);
        });
    }

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startGroup("PACKED_FSE_BITMAP");

        handler->value("PARENT_ALLOCATOR", &header_.allocator_offset());

        handler->value("SIZE", &size_);
        handler->value("MAX_SIZE", &max_size_);

        handler->startGroup("DATA", size());

        handler->symbols("SYMBOLS", buffer_, size(), BitsPerSymbol);

        handler->endGroup();

        handler->endGroup();

        return VoidResult::of();
    }

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(header_.serialize(buf));

        FieldFactory<int32_t>::serialize(buf, size_);
        FieldFactory<int32_t>::serialize(buf, max_size_);

        FieldFactory<Value>::serialize(buf, buffer_, symbols_buffer_size());

        return VoidResult::of();
    }

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(header_.deserialize(buf));

        FieldFactory<int32_t>::deserialize(buf, size_);
        FieldFactory<int32_t>::deserialize(buf, max_size_);

        FieldFactory<Value>::deserialize(buf, buffer_, symbols_buffer_size());

        return VoidResult::of();
    }

private:
    int32_t symbols_buffer_size() const
    {
        int32_t block_size  = this->block_size();
        int32_t buffer_size = block_size - sizeof(MyType);

        int32_t bit_size    = buffer_size * 8;
        int32_t byte_size   = PackedAllocatable::roundUpBitsToAlignmentBlocks(bit_size);

        return byte_size / sizeof(Value);
    }

    void move(Value* symbols, int32_t from, int32_t to, int32_t lenght) const
    {
        MoveBits(symbols, symbols, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }

    void move(const Value* src, Value* dst, int32_t from, int32_t to, int32_t lenght) const
    {
        MoveBits(src, dst, from * BitsPerSymbol, to * BitsPerSymbol, lenght * BitsPerSymbol);
    }

    void enlargeData(int32_t elements) {
        enlarge(elements);
    }

    void enlarge(int32_t elements)
    {
        Allocator* alloc = header_.allocator();
        int32_t amount = PackedAllocatable::roundUpBitsToAlignmentBlocks((size_ + elements) * BitsPerSymbol);

        int32_t new_size = alloc->resizeBlock(this, amount + empty_size());
        max_size_ = (new_size - empty_size()) * 8 / BitsPerSymbol;
    }

    void shrink(int32_t elements)
    {
        MEMORIA_V1_ASSERT(size_, >=, elements);

        Allocator* alloc = header_.allocator();
        int32_t amount = PackedAllocatable.roundUpBitsToAlignmentBlocks((size_ - elements) * BitsPerSymbol);

        int32_t new_size = alloc->resizeBlock(this, amount + empty_size());
        max_size_ = (new_size - empty_size()) * 8 / BitsPerSymbol;
    }
};

}
