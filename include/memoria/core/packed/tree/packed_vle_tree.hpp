
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_TREE2_HPP_
#define MEMORIA_CORE_PACKED_VLE_TREE2_HPP_

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>
#include <memoria/core/packed/tree/packed_tree_walkers.hpp>

#include <memoria/core/tools/exint_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/i64_codec.hpp>

#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/static_array.hpp>

namespace memoria {

using namespace vapi;




template <typename Value>
class VLETreeValueDescr {
    Value value_;
    Int pos_;
    Int idx_;
    Value prefix_;
public:
    VLETreeValueDescr(BigInt value, Int pos, Int idx, Value prefix = 0):
        value_(value),
        pos_(pos),
        idx_(idx),
        prefix_(prefix)
        {}

    Value value() const     {return value_;}
    Int pos() const         {return pos_;}
    Int idx() const         {return idx_;}
    Value prefix() const    {return prefix_;}
};


template <typename Types_>
class PkdVTree: public PackedAllocator
{
    typedef PackedAllocator                                                     Base;

public:
    static const UInt VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PkdVTree<Types>                                                     MyType;

    typedef PackedAllocator                                                     Allocator;

    typedef typename Types::IndexValue                                          IndexValue;
    typedef typename Types::Value                                               Value;

    typedef UBigInt                                                             OffsetsType;


    typedef typename Types::template Codec<Value>                               Codec;
    typedef typename Codec::BufferType                                          BufferType;

    typedef Int                                                                 LayoutValue;

    static const Int BranchingFactor        = Types::BranchingFactor;
    static const Int ValuesPerBranch        = Types::ValuesPerBranch;
    static const Int Indexes                = 2;
    static const Int Blocks                 = Types::Blocks;
    static const Int IOBatchSize            = Blocks <= 2 ? 256: (Blocks <= 4 ? 64 : (Blocks <= 16 ? 16 : 4));
    static const bool FixedSizeElement      = false;

    static const Int BITS_PER_OFFSET        = Codec::BitsPerOffset;

    enum {
        METADATA, OFFSETS, LAYOUT, INDEX, VALUES
    };

    typedef core::StaticVector<Int, Blocks>                                     Dimension;
    typedef core::StaticVector<Dimension, 2>                                    BlockRange;
    typedef core::StaticVector<IndexValue, Blocks>                              Values;
    typedef core::StaticVector<IndexValue, Blocks + 1>                          Values2;

    typedef PackedTreeTools<BranchingFactor, ValuesPerBranch>                   TreeTools;

    typedef VLETreeValueDescr<IndexValue>                                       ValueDescr;

    typedef FnAccessor<Value>                                                   ValueAccessor;
    typedef ConstFnAccessor<Value>                                              ConstValueAccessor;

    class Metadata {
        Int size_;
        Int data_size_;
        Int index_size_;
    public:
        Metadata() {}

        const Int& size() const         {return size_;};
        const Int& data_size() const    {return data_size_;};
        Int index_size() const          {return index_size_;};

        Int& size()                     {return size_;};
        Int& data_size()                {return data_size_;};
        Int& index_size()               {return index_size_;};

        template<typename> friend class PkdVTree;
    };

public:
    PkdVTree() {}

public:
    Metadata* metadata() {
        return Base::template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const {
        return Base::template get<Metadata>(METADATA);
    }

    Int& size() {return metadata()->size();}
    const Int& size() const {return metadata()->size();}

    Int raw_size() const {return size() * Blocks;}

    const Int index_size() const {return metadata()->index_size();}

    const Int& data_size() const {return metadata()->data_size();}

    Int& data_size() {return metadata()->data_size();}

    Int data_length() const
    {
        Int bit_length  = data_size() * Codec::ElementSize;
        Int byte_length = PackedAllocator::roundUpBitToBytes(bit_length);

        const Int b_size = sizeof(BufferType);

        return byte_length / b_size + (byte_length % b_size ? 1 : 0);
    }

    Int raw_capacity() const
    {
        return element_size(VALUES) * 8 / Codec::ElementSize;
    }

    static Int max_capacity(Int capacity)
    {
        Int bits    = capacity * Codec::ElementSize;
        Int bytes   = Base::roundUpBitsToAlignmentBlocks(bits);

        return bytes * 8 / Codec::ElementSize;
    }


    OffsetsType* offsetsBlock() {
        return Base::template get<OffsetsType>(OFFSETS);
    }

    const OffsetsType* offsetsBlock() const {
        return Base::template get<OffsetsType>(OFFSETS);
    }


    IndexValue* indexes(Int index_block)
    {
        return Base::template get<IndexValue>(INDEX) + index_block * index_size();
    }

    IndexValue* sizes() {
        return indexes(0);
    }

    const IndexValue* indexes(Int index_block) const
    {
        return Base::template get<IndexValue>(INDEX) + index_block * index_size();
    }


    const LayoutValue* index_layout() const
    {
        return Base::template get<LayoutValue>(LAYOUT);
    }

    LayoutValue* index_layout()
    {
        return Base::template get<LayoutValue>(LAYOUT);
    }

    static Int index_layout_size(Int capacity)
    {
        return capacity > ValuesPerBranch ? TreeTools::compute_layout_size(capacity) : 0;
    }


    const IndexValue* sizes() const {
        return indexes(0);
    }

    BufferType* values()
    {
        return Base::template get<BufferType>(VALUES);
    }

    const BufferType* values() const
    {
        return Base::template get<BufferType>(VALUES);
    }


    Int block_size() const
    {
        return Base::block_size();
    }

    Int block_size(const MyType* other) const
    {
        Int my_max      = this->raw_capacity();
        Int other_max   = other->raw_capacity();

        return block_size(my_max + other_max);
    }

    static Int block_size(Int tree_capacity)
    {
        Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        Int max_tree_capacity = max_capacity(tree_capacity);

        Int offsets_length  = Base::roundUpBytesToAlignmentBlocks(getOffsetsBlockLength(max_tree_capacity));

        Int index_size      = MyType::index_size(max_tree_capacity);
        Int index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * Indexes * sizeof(IndexValue));

        Int layout_size     = MyType::index_layout_size(max_tree_capacity);
        Int layout_length   = Base::roundUpBytesToAlignmentBlocks(layout_size * sizeof(LayoutValue));

        Int values_length   = Base::roundUpBitsToAlignmentBlocks(tree_capacity * Codec::ElementSize);

        return Base::block_size(metadata_length + offsets_length + layout_length + index_length + values_length, 5);
    }

    static Int estimate_block_size(Int tree_capacity, Int density_hi = 1000, Int density_lo = 333)
    {
        Int max_tree_capacity = (tree_capacity * Blocks * density_hi) / density_lo;
        return block_size(max_tree_capacity);
    }

    static Int empty_size()
    {
        return block_size(0);
    }

    static Int max_tree_size(Int block_size)
    {
        return FindTotalElementsNumber2(block_size, InitFn());
    }

    static Int elements_for(Int block_size)
    {
        return max_tree_size(block_size);
    }

    std::pair<Int, Int>
    density() const
    {
        Int data_size   = this->data_size();
        Int raw_size    = this->raw_size();

        return std::pair<Int, Int>(data_size, raw_size);
    }


    // ================================= Reindexing ======================================== //

private:

    class ReindexFn: public ReindexFnBase<MyType> {

        typedef ReindexFnBase<MyType>               Base;

        const BufferType* values_;

    public:
        ReindexFn(MyType& me): Base(me)
        {
            values_ = me.values();
        }

        void buildFirstIndexLine(Int index_level_start, Int index_level_size)
        {
            auto& me = Base::tree();
            auto& indexes = Base::indexes_;

            Int limit       = me.data_size();
            Int data_pos    = 0;

            Codec codec;

            Int idx         = 0;
            Int block_start = 0;

            Int total_size = 0;

            Int cnt = 0;

            for (; idx < index_level_size && data_pos < limit; idx++, block_start += ValuesPerBranch)
            {
                Int next        = block_start + ValuesPerBranch;
                Int local_limit = next <= limit ? next : limit;

                me.offset(idx) = data_pos - block_start;

                IndexValue size_cell  = 0;
                IndexValue value_cell = 0;

                while (data_pos < local_limit)
                {
                    Value value;

                    Int len = codec.decode(values_, value, data_pos, limit);
                    MEMORIA_ASSERT(len, >, 0);
                    cnt++;

                    value_cell += value;
                    size_cell++;

                    data_pos += len;

                    total_size++;
                }

                indexes[0][idx + index_level_start] = size_cell;
                indexes[1][idx + index_level_start] = value_cell;
            }

            for (;idx < index_level_size; idx++)
            {
                me.offset(idx) = 0;
            }
        }

        template <typename T>
        void buildFirstIndexLine(Int index_level_start, Int index_level_size, const T* data, Int data_size)
        {
            auto& me = Base::tree();
            auto& indexes = Base::indexes_;

            Int limit       = me.data_size();
            Int data_pos    = 0;

            Codec codec;

            Int idx         = 0;
            Int block_start = 0;

            for (Int cnt = 0; idx < index_level_size && data_pos < limit; idx++, block_start += ValuesPerBranch)
            {
                Int next        = block_start + ValuesPerBranch;
                Int local_limit = next <= limit ? next : limit;

                me.offset(idx) = data_pos - block_start;

                IndexValue size_cell  = 0;
                IndexValue value_cell = 0;

                while (data_pos < local_limit)
                {
                    Value value = data[cnt];
                    Int len = codec.length(value);

                    cnt++;

                    value_cell += value;
                    size_cell++;

                    data_pos += len;
                }

                indexes[0][idx + index_level_start] = size_cell;
                indexes[1][idx + index_level_start] = value_cell;
            }

            for (;idx < index_level_size; idx++)
            {
                me.offset(idx) = 0;
            }
        }
    };



    class CheckFn: public CheckFnBase<MyType> {

        typedef CheckFnBase<MyType>                 Base;

        const BufferType* values_;
    public:
        Int data_size_  = 0;
        Int size_       = 0;

    public:
        CheckFn(const MyType& me): Base(me)
        {
            values_ = me.values();
        }

        void buildFirstIndexLine(Int index_level_start, Int index_level_size)
        {
            auto& me = Base::me_;
            auto& indexes = Base::indexes_;

            Int limit       = Base::me_.data_size();
            Int data_pos    = 0;

            Codec codec;

            Int idx = 0, block_start = 0;

            for (; idx < index_level_size && data_pos < limit; idx++, block_start += ValuesPerBranch)
            {
                Int next        = block_start + ValuesPerBranch;
                Int local_limit = next <= limit ? next : limit;

                Int vpb = ValuesPerBranch;

                MEMORIA_ASSERT(me.offset(idx), <=, vpb);
                MEMORIA_ASSERT(me.offset(idx), ==, data_pos - block_start);

                IndexValue size_cell  = 0;
                IndexValue value_cell = 0;

                while (data_pos < local_limit)
                {
                    Value value;
                    Int len = codec.decode(values_, value, data_pos, limit);
                    MEMORIA_ASSERT(len, >, 0);

                    value_cell += value;
                    size_cell++;

                    data_pos += len;
                }

                size_ += size_cell;

                MEMORIA_ASSERT(indexes[0][idx + index_level_start], ==, size_cell);
                MEMORIA_ASSERT(indexes[1][idx + index_level_start], ==, value_cell);
            }

            for (;idx < index_level_size; idx++)
            {
                MEMORIA_ASSERT(indexes[0][idx + index_level_start], ==, 0);
                MEMORIA_ASSERT(indexes[1][idx + index_level_start], ==, 0);

                MEMORIA_ASSERT(me.offset(idx), ==, 0);
            }

            data_size_ = data_pos;
        }

        void checkData()
        {
            Int limit = Base::tree().data_size();

            Codec codec;

            while (data_size_ < limit)
            {
                Value value;
                data_size_ += codec.decode(values_, value, data_size_, limit);
                size_ ++;
            }
        }
    };


public:

    void reindex()
    {
        ReindexFn fn(*this);
        TreeTools::reindex(fn);
    }

    template <typename T>
    void reindexBlock(const T* data, Int size)
    {
        ReindexFn fn(*this);
        TreeTools::reindexBlock(fn, data, size);
    }

    void check() const
    {
        CheckFn fn(*this);
        TreeTools::check(fn);

        MEMORIA_ASSERT(data_size(), <=, raw_capacity());
        MEMORIA_ASSERT(fn.data_size_, ==, data_size());
        MEMORIA_ASSERT(fn.size_, ==, raw_size());
    }


    // ==================================== Value ========================================== //

private:

    class GetValueOffsetFn: public GetValueOnlyOffsetFnBase<MyType, GetValueOffsetFn> {
        typedef GetValueOnlyOffsetFnBase<MyType, GetValueOffsetFn> Base;

        Int     max_;
        bool    has_index_;

        const LayoutValue* layout_;

    public:
        GetValueOffsetFn(const MyType& me, Int limit):
            Base(me, limit),
            max_(me.data_size()),
            has_index_(me.has_index()),
            layout_(me.index_layout())
        {}

        Int max() const {
            return max_;
        }

        bool has_index() const {
            return has_index_;
        }

        const LayoutValue* index_layout() const {
            return layout_;
        }

        void processIndexes(Int start, Int end) {}
    };

public:
    Int value_offset(Int idx) const
    {
        GetValueOffsetFn fn(*this, idx);

        Int pos = TreeTools::find(fn);

        return pos;
    }

    Int locate(Int idx) const
    {
        Codec codec;

        Int limit = this->data_size();
        auto values = this->values();

        Int pos = 0;
        Int c = 0;

        for (c = 0; c < idx && pos < limit; c++)
        {
            Int len = codec.length(values, pos, limit);
            pos += len;
        }

        if (c == idx) {
            return pos;
        }
        else {
            return limit;
        }
    }


    Value getValue(Int idx) const
    {
        const auto* values_ = values();

        if (idx >= raw_size())
        {
            int a = 0; a++;
        }

        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <, raw_size());

        Int pos = value_offset(idx);

        Codec codec;
        Value value;

        MEMORIA_ASSERT(pos, <, data_size());

        codec.decode(values_, value, pos, data_size());
        return value;
    }

    Value getValue(Int block, Int idx) const
    {
        Int value_idx = size() * block + idx;

        return getValue(value_idx);
    }


    Int setValue(Int idx, Value value)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, raw_size());

        auto values = this->values();

        Int pos = value_offset(idx);

        Int limit = this->raw_capacity();

        MEMORIA_ASSERT(pos, <=, limit);

        Int total_size = data_size();

        Codec codec;

        Int value_len = codec.length(value);

        Int stored_value_len = codec.length(values, pos, limit);
        Int delta            = value_len - stored_value_len;

        if (delta > 0)
        {
            Int capacity = this->raw_capacity() - total_size;

            if (capacity < delta)
            {
                enlarge(delta);
            }
        }

        auto* meta = this->metadata();
        values = this->values();

        Int data_size   = meta->data_size();

        MEMORIA_ASSERT(pos + value_len, <=, data_size + delta);

        codec.move(values, pos + stored_value_len, pos + value_len, total_size - (pos + stored_value_len));

        codec.encode(values, value, pos, data_size + delta);

        meta->data_size() += delta;

        reindex();

        return 0;
    }


    Int setValue(Int block, Int idx, Value value)
    {
        Int value_idx = size() * block + idx;

        return setValue(value_idx, value);
    }

    void setValues(Int idx, const core::StaticVector<Value, Blocks>& values)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            setValue(block, idx, values[block]);
        }
    }

    void addValues(Int idx, const core::StaticVector<Value, Blocks>& values)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            if (values[block] != 0)
            {
                Value val = getValue(block, idx);
                setValue(block, idx, val + values[block]);
            }
        }
    }

    void addValue(Int block, Int idx, Value value)
    {
        if (value != 0)
        {
            Value val = getValue(block, idx);
            setValue(block, idx, val + value);
        }
    }

    BigInt setValue1(Int block, Int idx, Value value)
    {
        if (value != 0)
        {
            Value val = getValue(block, idx);
            setValue(block, idx, value);

            return val - value;
        }
        else {
            return 0;
        }
    }



    void appendValue(Value value)
    {
        Int len = setValue(size(), value);
        if (len == 0)
        {
            size()++;
            reindex();
        }
        else {
            throw Exception(MA_SRC, SBuf()<<"Out of the memory block for value at "<<size());
        }
    }


    ValueAccessor value(Int idx)
    {
        return ValueAccessor(
            [this, idx]() {return this->getValue(idx);},
            [this, idx](const Value& value) {
                if (this->setValue(idx, value) > 0)
                {
                    throw Exception(MA_SRC, SBuf()<<"Out of the memory block for value at "<<idx);
                }
            }
        );
    }

    ValueAccessor value(Int block, Int idx)
    {
        return ValueAccessor(
            [this, block, idx]() {return this->getValue(block, idx);},
            [this, block, idx](const Value& value) {
                if (this->setValue(block, idx, value) > 0)
                {
                    throw Exception(MA_SRC, SBuf()<<"Out of the memory block for value at "<<idx);
                }
            }
        );
    }


    ConstValueAccessor value(Int idx) const
    {
        return ConstValueAccessor(
            [this, idx]() {return this->getValue(idx);}
        );
    }

    ConstValueAccessor value(Int block, Int idx) const
    {
        return ConstValueAccessor(
                [this, block, idx]() {return this->getValue(block, idx);}
        );
    }

    // =================================== Offsets ======================================== //

    Int offsets() const
    {
        Int blocks = getValueBlocks();
        return blocks > 1 ? blocks : 0;
    }

    BitmapAccessor<OffsetsType*, Int, BITS_PER_OFFSET>
    offset(Int idx)
    {
        return BitmapAccessor<OffsetsType*, Int, BITS_PER_OFFSET>(offsetsBlock(), idx);
    }

    BitmapAccessor<const OffsetsType*, Int, BITS_PER_OFFSET>
    offset(Int idx) const
    {
        return BitmapAccessor<const OffsetsType*, Int, BITS_PER_OFFSET>(offsetsBlock(), idx);
    }

    // =============================== Initialization ====================================== //


private:
    struct InitFn {
        Int block_size(Int items_number) const {
            return MyType::block_size(items_number);
        }

        Int max_elements(Int block_size)
        {
            return block_size * 8;
        }
    };

public:
    // ================================= Allocation ======================================== //

    void init(Int block_size)
    {
        Base::init(block_size, 5);

        Metadata* meta = Base::template allocate<Metadata>(METADATA);

        Int max_size        = FindTotalElementsNumber2(block_size, InitFn());

        meta->size()        = 0;
        meta->data_size()   = 0;
        meta->index_size()  = MyType::index_size(max_size);

        Int offsets_length  = getOffsetsBlockLength(max_size);
        Base::template allocateArrayByLength<OffsetsType>(OFFSETS, offsets_length);

        Int layout_size = MyType::index_layout_size(max_size);
        Base::template allocateArrayBySize<LayoutValue>(LAYOUT, layout_size);
        TreeTools::buildIndexTreeLayout(index_layout(), max_size, layout_size);

        Int index_size = meta->index_size();
        Base::template allocateArrayBySize<IndexValue>(INDEX, index_size * Indexes);

        Int values_block_length = Base::roundUpBitsToAlignmentBlocks(max_size * Codec::ElementSize);
        Base::template allocateArrayByLength<BufferType>(VALUES, values_block_length);
    }

    void inits(Int capacity)
    {
        Int block_size = MyType::block_size(capacity);

        Base::init(block_size, 5);

        Metadata* meta = Base::template allocate<Metadata>(METADATA);

        Int max_capacity = MyType::max_capacity(capacity);

        meta->size()        = 0;
        meta->data_size()   = 0;
        meta->index_size()  = MyType::index_size(max_capacity);

        Int offsets_length  = getOffsetsBlockLength(max_capacity);
        Base::template allocateArrayByLength<OffsetsType>(OFFSETS, offsets_length);

        Int layout_size = MyType::index_layout_size(max_capacity);
        Base::template allocateArrayBySize<LayoutValue>(LAYOUT, layout_size);
        TreeTools::buildIndexTreeLayout(index_layout(), max_capacity, layout_size);

        Int index_size = meta->index_size();
        Base::template allocateArrayBySize<IndexValue>(INDEX, index_size * Indexes);

        Int values_block_length = Base::roundUpBitsToAlignmentBlocks(max_capacity * Codec::ElementSize);
        Base::template allocateArrayByLength<BufferType>(VALUES, values_block_length);
    }

    void init() {
        inits(0);
    }

    void clear()
    {
        Base::resizeBlock(VALUES,   0);
        Base::resizeBlock(INDEX,    0);
        Base::resizeBlock(OFFSETS,  0);
        Base::resizeBlock(LAYOUT,   getOffsetsBlockLength(0));

        Metadata* meta = this->metadata();
        meta->size()        = 0;
        meta->data_size()   = 0;
        meta->index_size()  = 0;
    }

    bool check_capacity(Int size) const
    {
        MEMORIA_ASSERT_TRUE(size >= 0);

        Int total_size          = this->data_size() + size;
        Int total_block_size    = MyType::block_size(total_size);
        Int my_block_size       = this->block_size();
        Int delta               = total_block_size - my_block_size;

        auto alloc = this->allocator();

        return alloc->free_space() >= delta;
    }

    bool has_index() const {
        return index_size() > 0;
    }

    bool has_index_layout() const {
        return element_size(LAYOUT) > 0;
    }



    void insertSpace(Int, Int) {
        throw Exception(MA_SRC, "Method insertSpace(Int, Int) is not implemented for PkdVTree");
    }



    Dimension insertSpace2(Int idx, const Dimension& lengths)
    {
        MEMORIA_ASSERT_TRUE(idx >= 0);

        Int total       = lengths.sum();
        Int size        = this->size();
        Int max_size    = this->raw_capacity();
        Int max         = this->data_size();

        Int capacity    = max_size - max;

        if (total > capacity)
        {
            enlarge(total - capacity);
        }

        Codec codec;

        Dimension starts;
        Dimension remainders;

        for (Int block = 0, inserted_length = 0; block < Blocks; inserted_length += lengths[block], block++)
        {
            Int offset          = block * size + idx;
            Int offset_pos      = this->value_offset(offset);

            starts[block]       = offset_pos + inserted_length;
            remainders[block]   = max + inserted_length - starts[block];
        }

        auto* values = this->values();

        for (Int block = 0; block < Blocks; block++)
        {
            Int start       = starts[block];
            Int end         = starts[block] + lengths[block];
            Int remainder   = remainders[block];

            MEMORIA_ASSERT(remainder, >=, 0);

            codec.move(values, start, end, remainder);
        }

        this->data_size() += total;

        return starts;
    }


    Dimension insertSpace(Int idx, const Dimension& lengths)
    {
        MEMORIA_ASSERT_TRUE(idx >= 0);

        Int size        = this->size();
        Int max_size    = this->raw_capacity();
        Int max         = this->data_size();

        Int capacity    = max_size - max;

        Dimension lengths_s;

        lengths_s[0] = lengths[0];

        for (Int c = 1; c < Blocks; c++)
        {
            lengths_s[c] += lengths[c] + lengths_s[c - 1];
        }

        Int total = lengths_s[Blocks - 1];

        if (total > capacity)
        {
            enlarge(total - capacity);
        }

        Codec codec;

        Dimension starts;
        Dimension ends;
        Dimension t_lengths;
        Dimension starts_i;

        for (Int block = 0; block < Blocks; block++)
        {
            Int offset      = block * size + idx;
            starts[block]   = this->value_offset(offset);
        }

        for (Int block = 0; block < Blocks - 1; block++)
        {
            t_lengths[block]= starts[block + 1] - starts[block];
            ends[block]     += starts[block] + lengths_s[block];
        }

        ends[Blocks - 1]        = starts[Blocks - 1] + lengths_s[Blocks - 1];
        t_lengths[Blocks - 1]   = max - starts[Blocks - 1];

        starts_i[0] = starts[0];

        for (Int c = 1; c < Blocks; c++)
        {
            starts_i[c] = starts[c] + lengths_s[c - 1];
        }

        auto* values = this->values();

        for (Int block = Blocks - 1; block >= 0; block--)
        {
            Int start   = starts[block];
            Int end     = ends[block];
            Int length  = t_lengths[block];

            if (length > 0)
            {
                codec.move(values, start, end, length);
            }
        }

        this->data_size() += total;

        return starts_i;
    }

    void remove(Int start, Int end)
    {
        Int size = this->size();
        Int max  = this->data_size();

        MEMORIA_ASSERT_TRUE(start >= 0);

        Int room_length = end - start;

        MEMORIA_ASSERT_TRUE(room_length >= 0);
        MEMORIA_ASSERT(room_length, <= , size - start);

        Codec codec;

        Dimension starts;
        Dimension ends;

        for (Int block = 0; block < Blocks; block++)
        {
            starts[block]   = this->value_offset(block * size + start);
            ends[block]     = this->value_offset(block * size + end);
        }

        auto* values = this->values();

        Int total = 0;

        for (Int block = 0; block < Blocks - 1; block++)
        {
            Int start   = ends[block];
            Int length  = starts[block + 1] - start;
            Int end     = starts[block] - total;

            if (length > 0)
            {
                codec.move(values, start, end, length);
            }

            total += ends[block] - starts[block];
        }

        if (max - ends[Blocks - 1] > 0)
        {
            codec.move(values, ends[Blocks - 1], starts[Blocks - 1] - total, max - ends[Blocks - 1]);
        }

        total += ends[Blocks - 1] - starts[Blocks - 1];

        this->size()        -= room_length;
        this->data_size()   -= total;

        if (this->size() > 0)
        {
//          shrink(total);
            pack();
        }
        else {
            clear();
        }
    }

    void removeSpace(Int start, Int end) {
        remove(start, end);
    }


    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
        throw Exception(MA_SRC, "copyTo() haven't not been fully implemented for this data structure yet");
    }

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        Int size = this->size();

        Int total = this->value_offset(size * Blocks);
        other->enlarge(total);

        Codec codec;
        codec.copy(values(), 0, other->values(), 0, total);

        other->size()       = size;
        other->data_size()  = this->data_size();

        other->reindex();
    }

    void splitTo(MyType* other, Int idx)
    {
        Int size        = this->size();
        Int other_size  = other->size();

        Dimension lengths;

        for (Int block = 0; block < Blocks; block++)
        {
            lengths[block] = this->data_length(block, idx, size);
        }

        Dimension other_starts;

        for (Int block = 1, sum = lengths[0]; block < Blocks; block++)
        {
            Int start           = other->value_offset(other_size * block);
            other_starts[block] = start + sum;
            sum                 += lengths[block];
        }

        other->insertSpace(0, lengths);

        Codec codec;
        for (Int block = 0; block < Blocks; block++)
        {
            Int pos = this->value_offset(size * block + idx);
            Int tgt_pos = other_starts[block];

            codec.copy(values(), pos, other->values(), tgt_pos, lengths[block]);
        }

        other->size()       += size - idx;

        other->reindex();

        removeSpace(idx, size);
    }

    void mergeWith(MyType* other)
    {
        Int my_size     = this->size();
        Int other_size  = other->size();


        Dimension lengths;

        for (Int block = 0, sum = 0; block < Blocks; block++)
        {
            lengths[block] = this->value_offset((block + 1) * my_size) - sum;
            sum += lengths[block];
        }

        Dimension other_starts;

        for (Int block = 0, sum = 0; block < Blocks; block++)
        {
            Int block_end       = other->value_offset((block + 1) * other_size);
            other_starts[block] = block_end + sum;
            sum                 += lengths[block];
        }

        other->insertSpace(other_size, lengths);

        Codec codec;

        for (Int block = 0; block < Blocks; block++)
        {
            Int my_start    = this->value_offset(block * my_size);
            Int other_start = other_starts[block];

            codec.copy(values(), my_start, other->values(), other_start, lengths[block]);
        }

        other->size() += my_size;

        other->reindex();

        this->clear();
    }

    void clear(Int, Int) {}


    // ==================================== IO ============================================= //


    static Int computeDataLength(const Values& values)
    {
        Codec codec;
        Int length = 0;

        for (Int c = 0; c < Blocks; c++)
        {
            length += codec.length(values[c]);
        }

        return length;
    }

    float estimateEntropy(Int start, Int end) const
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <, end);
        MEMORIA_ASSERT(end, <=, size());

        float total_length  = 0;
        float total_count   = (end - start) * Blocks;

        for (Int c = 0; c < Blocks; c++)
        {
            Int start_idx   = this->value_offset(start);
            Int end_idx     = this->value_offset(end);

            total_length += (end_idx - start_idx);
        }

        return total_length / total_count;
    }

    float estimateEntropy() const
    {
        Int total_count = size() * Blocks;

        Int start_idx   = 0;
        Int end_idx     = this->value_offset(total_count);

        float total_length = (end_idx - start_idx);

        return total_length / total_count;
    }


    void insert(Int idx, Int length, std::function<Values ()> provider)
    {
        Values values[IOBatchSize];

        Int to_write_local  = length;

        while (to_write_local > 0)
        {
            Int batch_size = to_write_local > IOBatchSize ? IOBatchSize : to_write_local;

            for (Int c = 0; c < batch_size; c++)
            {
                values[c] = provider();
            }

            insertData(values, idx, batch_size);

            idx             += batch_size;
            to_write_local  -= batch_size;
        }
    }


    void update(Int start, Int end, std::function<Values ()> provider)
    {
        remove(start, end);
        insert(start, end - start, provider);
    }


    Int insert(Int idx, std::function<bool (Values&)> provider)
    {
        Values vals;
        Int cnt = 0;

        Codec codec;

        while(provider(vals))
        {
            Int total_length = 0;

            for (Int b = 0; b < Blocks; b++)
            {
                total_length += codec.length(vals[b]);
            }

            if (check_capacity(total_length))
            {
                insertData(&vals, idx, 1);

                idx++;
                cnt++;
            }
            else {
                break;
            }
        }

        return cnt;
    }

    void insert(Int idx, const Values& values)
    {
        insertData(&values, idx, 1);
    }


    void insert(IData* data, Int pos, Int length)
    {
        MEMORIA_ASSERT(pos, <=, size());

        IDataSource<Values>* src = static_cast<IDataSource<Values>*>(data);

        IDataAPI api_type = src->api();

        Values values[IOBatchSize];
        BigInt to_write_local = length;

        if (api_type == IDataAPI::Batch || api_type == IDataAPI::Both)
        {
            while (to_write_local > 0)
            {
                SizeT remainder     = src->getRemainder();
                SizeT batch_size    = remainder > IOBatchSize ? IOBatchSize : remainder;

                SizeT processed     = src->get(&values[0], 0, batch_size);

                insertData(values, pos, processed);

                pos             += processed;
                to_write_local  -= processed;
            }
        }
        else {
            while (to_write_local > 0)
            {
                SizeT remainder     = src->getRemainder();
                SizeT batch_size    = remainder > IOBatchSize ? IOBatchSize : remainder;

                for (Int c = 0; c < batch_size; c++)
                {
                    values[c] = src->get();
                }

                insertData(values, pos, batch_size);

                pos             += batch_size;
                to_write_local  -= batch_size;
            }
        }

        reindex();
    }

    template <Int LineWidth, typename T>
    void insertBlock(const T* data, Int blocks)
    {
        auto values = this->values();
        Codec codec;

        Int pos = 0;

        for (Int block = 0; block < Blocks; block++)
        {
            for (Int idx = 0; idx < blocks; idx++)
            {
                Int data_idx = block * LineWidth + idx;

                pos += codec.encode(values, data[data_idx], pos);
            }
        }

        this->data_size()   = pos;
        this->size()        = blocks;

        reindex();
    }


    void append(IData* src, Int pos, Int length)
    {
        insert(src, size(), length);
    }

    void update(IData* data, Int pos, Int length)
    {
        removeSpace(pos, pos + length);
        insert(data, pos, length);
    }





    void read(IData* data, Int pos, Int length)
    {
        IDataTarget<Values>* tgt = static_cast<IDataTarget<Values>*>(data);

        IDataAPI api_type = tgt->api();

        Values values[IOBatchSize];
        BigInt to_read = length;

        if (api_type == IDataAPI::Batch || api_type == IDataAPI::Both)
        {
            while (to_read > 0)
            {
                SizeT remainder     = tgt->getRemainder();
                SizeT batch_size    = remainder > IOBatchSize ? IOBatchSize : remainder;

                readData(values, pos, batch_size);

                Int local_pos = 0;
                Int to_read_local = batch_size;

                while (to_read_local > 0)
                {
                    SizeT processed = tgt->put(&values[0], local_pos, batch_size);

                    local_pos       += processed;
                    to_read_local   -= processed;
                }

                pos     += batch_size;
                to_read -= batch_size;
            }
        }
        else {
            while (to_read > 0)
            {
                SizeT remainder     = tgt->getRemainder();
                SizeT batch_size    = remainder > IOBatchSize ? IOBatchSize : remainder;

                readData(values, pos, batch_size);

                for (Int c = 0; c < batch_size; c++)
                {
                    tgt->put(values[c]);
                }

                pos     += batch_size;
                to_read -= batch_size;
            }
        }
    }



    void read(Int start, Int end, std::function<void (const Values&)> consumer) const
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, end);
        MEMORIA_ASSERT(end, <=, size());

        Values values[IOBatchSize];

        Int to_read = end - start;
        Int pos     = start;

        while (to_read > 0)
        {
            SizeT batch_size    = to_read > IOBatchSize ? IOBatchSize : to_read;

            readData(values, pos, batch_size);

            for (Int c = 0; c < batch_size; c++)
            {
                consumer(values[c]);
            }

            pos     += batch_size;
            to_read -= batch_size;
        }
    }



    // ==================================== Sum ============================================ //



    IndexValue sum(Int block) const
    {
        return sum(block, size());
    }

    IndexValue sum(Int block, Int to) const
    {
        Int base = block * size();
        return raw_sum(base, base + to);
    }


    IndexValue sumWithoutLastElement(Int block) const
    {
        return sum(block, size() - 1);
    }

    IndexValue sum(Int block, Int from, Int to) const
    {
        Int base = block * size();
        return raw_sum(base + to).value() - raw_sum(base + from).value();
    }


    Values sums(Int from, Int to) const
    {
        Values vals;

        for (Int block = 0; block < Blocks; block++)
        {
            vals[block] = sum(block, from, to);
        }

        return vals;
    }




    Values sums() const
    {
        Values vals;

        sumsSmall<0>(vals);

        return vals;
    }

    Values2 sums2() const
    {
        Values vals;

        for (Int block = 0; block < Blocks; block++)
        {
            vals[block + 1] = sum(block);
        }

        vals[0] = size();

        return vals;
    }

    Values2 sums2(Int from, Int to) const
    {
        Values2 vals;

        for (Int block = 0; block < Blocks; block++)
        {
            vals[block + 1] = sum(block, from, to);
        }

        vals[0] = to - from;

        return vals;
    }


    void sums(Int from, Int to, Values& values) const
    {
        values += sums(from, to);
    }

    void sums(Int from, Int to, Values2& values) const
    {
        values[0] += to - from;
        values.sumUp(sums(from, to));
    }

    void sums(Values& values) const
    {
        sumsSmall<0>(values);
    }

    void sums(Values2& values) const
    {
        values[0] += size();

        sumsSmall<1>(values);
    }

    void sums(Int idx, Values2& values) const
    {
        values[0]++;

        for (Int c = 1; c < Values2::Indexes; c++)
        {
            values[c] = this->getValue(c - 1, idx);
        }
    }

    void sums(Int idx, Values& values) const
    {
        for (Int c = 0; c < Values::Indexes; c++)
        {
            values[c] = this->getValue(c, idx);
        }
    }

    // ==================================== Find =========================================== //




    template <template <typename, typename> class Comparator>
    ValueDescr find(Value value) const
    {
        const Metadata* meta = this->metadata();

        FindElementFn<MyType, Comparator> fn(
            *this,
            value,
            meta->index_size(),
            meta->size(),
            meta->data_size()
        );

        Int pos = TreeTools::find_fw(fn);

        Codec codec;
        Value actual_value;

        auto values_ = this->values();
        codec.decode(values_, actual_value, pos, meta->max_size());

        return ValueDescr(actual_value, pos, fn.position());
    }

    template <template <typename, typename> class Comparator>
    ValueDescr findForwardT(Int block, Int start, IndexValue val) const
    {
        const Metadata* meta = this->metadata();

        Int size        = meta->size();
        Int block_start = block * size;

        auto prefix = raw_sum(block_start + start).value();

        FindElementFn<MyType, Comparator> fn(
                *this,
                prefix + val,
                meta->data_size()
        );

        Int pos = TreeTools::find(fn);

        Int idx = fn.position() - block_start;

        if (idx < size)
        {
            Codec codec;
            Value actual_value;

            auto values = this->values();
            codec.decode(values, actual_value, pos, meta->data_size());

            return ValueDescr(actual_value, pos, idx >= 0 ? idx : 0, fn.sum() - prefix);
        }
        else {
            return ValueDescr(0, pos, size, sum(block, start, size));
        }
    }


    ValueDescr findGTForward(Int block, Int start, IndexValue val) const
    {
        return this->template findForwardT<PackedCompareLE>(block, start, val);
    }

    ValueDescr findGEForward(Int block, Int start, IndexValue val) const
    {
        return this->template findForwardT<PackedCompareLT>(block, start, val);
    }


    template <template <typename, typename> class Comparator>
    ValueDescr findBackwardT(Int block, Int start, IndexValue val) const
    {
        const Metadata* meta = this->metadata();

        Int size        = meta->size();
        Int block_start = block * size;

        auto prefix = raw_sum(block_start + start + 1).value();
        auto target = prefix - val;

        if (target >= 0)
        {
            FindElementFn<MyType, Comparator> fn(
                    *this,
                    target,
                    meta->data_size()
            );

            Int pos = TreeTools::find(fn);

            Int idx = fn.position() - block_start;

            if (idx > start)
            {
                return ValueDescr(0, start, 0, 0);
            }
            else if (idx >= 0)
            {
                Codec codec;
                Value actual_value;

                const auto* values = this->values();
                codec.decode(values, actual_value, pos, meta->data_size());

                return ValueDescr(actual_value, pos, idx, prefix - (fn.sum() + actual_value));
            }
            else {
                return ValueDescr(0, -1, -1, prefix - raw_sum(block_start).value());
            }
        }
        else {
            return ValueDescr(0, -1, -1, prefix);
        }
    }

    ValueDescr findGTBackward(Int block, Int start, IndexValue val) const
    {
        return this->template findBackwardT<PackedCompareLE>(block, start, val);
    }

    ValueDescr findGEBackward(Int block, Int start, IndexValue val) const
    {
        return this->template findBackwardT<PackedCompareLT>(block, start, val);
    }

    ValueDescr findForward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(block, start, val);
        }
        else {
            return findGEForward(block, start, val);
        }
    }

    ValueDescr findBackward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTBackward(block, start, val);
        }
        else {
            return findGEBackward(block, start, val);
        }
    }

    // ==================================== Dump =========================================== //

    void dumpBitmap() const
    {
        dumpBitmap(data_size());
    }

    void dumpBitmap(Int size) const
    {
        auto buffer = this->values();

        dumpSymbols<Value>(cout, size, 1, [&](Int idx){
            return GetBit(buffer, idx);
        });
    }




    void dumpValues(Int size) const
    {
        auto values = this->values();
        Int pos = 0;
        const Metadata* meta = this->metadata();
        Codec codec;

        dumpArray<Value>(cout, size, [&](Int) -> Value {
            Value value;
            pos += codec.decode(values, value, pos, meta->data_size());
            return value;
        });
    }

    void dumpSizes() const {
        Codec codec;
        Int limit = this->data_size();

        auto values = this->values();

        for (Int pos = 0, idx = 0; pos < limit; idx++)
        {
            IndexValue v;
            Int len = codec.decode(values, v, pos, limit);
            std::cout<<idx<<" "<<pos<<" "<<len<<" "<<std::hex<<v<<std::dec<<std::endl;
            pos += len;
        }
    }

    void dumpValues() const
    {
        Int cnt;
        Int limit = this->data_size();

        Codec codec;
        auto values = this->values();

        for (Int pos = 0; pos < limit;)
        {
            pos += codec.length(values, pos, limit);
            cnt++;
        }

        Int size = this->raw_size();

        dumpValues(cnt <= size ? cnt : size);
    }

    void dumpData() const
    {
        auto values = this->values();

        Int size = data_length();

        dumpArray<BufferType>(std::cout, size, [&](Int idx) {
            return values[idx];
        });
    }

    void dumpIndex(std::ostream& out = std::cout) const
    {
        Int idx_max = index_size();
        for (Int c = 0; c < idx_max; c++)
        {
            out<<c<<" ";
            for (Int block = 0; block < Indexes; block++)
            {
                out<<indexes(block)[c]<<" ";
            }
            out<<std::endl;
        }
    }

    void dumpLayout(std::ostream& out = std::cout) const
    {
        if (Base::element_size(LAYOUT) > 0)
        {
            auto layout = this->index_layout();

            for (Int c = 0; c < layout[0] + 1; c++)
            {
                out<<c<<" "<<layout[c]<<std::endl;
            }
        }
    }

    void dump(std::ostream& out = cout) const
    {
//      out<<"Layout: "<<std::endl;
//      Base::dump(out);

        out<<"size_         = "<<size()<<std::endl;
        out<<"data_size_    = "<<data_size()<<std::endl;
        out<<"capacity_     = "<<raw_capacity()<<std::endl;
        out<<"index_size_   = "<<index_size()<<std::endl;

        auto density = this->density();
        out<<"density_      = "<<density.first<<" "<<density.second<<" "
                               <<((float)density.first/(float)density.second)<<std::endl;

        out<<std::endl;

        out<<"Offsets:"<<std::endl;

        Int value_blocks = getValueBlocks(raw_capacity());

        if (value_blocks > 1)
        {
            dumpSymbols<Value>(out, value_blocks, Codec::BitsPerOffset, [this](Int idx) {
                return this->offset(idx);
            });
        }

        out<<std::endl;

        out<<"IndexLayout:"<<std::dec<<std::endl;

        dumpLayout(out);

        out<<"Indexes:"<<std::dec<<std::endl;

        dumpIndex(out);

        out<<std::endl;

        out<<"Data:"<<std::endl;

        const BufferType* values = this->values();

        size_t pos = 0;

        Codec codec;

        const Metadata* meta = metadata();

        dumpArray<Value>(out, meta->size() * Blocks, [&](Int) -> Value {
            Value value;
            pos += codec.decode(values, value, pos, meta->data_size());
            return value;
        });
    }


    // ================================= Serialization ============================= //


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startGroup("VLE_TREE");

        const Metadata* meta = this->metadata();

        handler->value("SIZE",          &meta->size_);

        Int raw_size = this->raw_size();
        handler->value("RAW_SIZE",      &raw_size);

        handler->value("DATA_SIZE",     &meta->data_size_);
        handler->value("INDEX_SIZE",    &meta->index_size_);

        Int max_size = this->raw_capacity();

        handler->value("MAX_DATA_SIZE", &max_size);

        if (has_index_layout())
        {
            auto layout = this->index_layout();

            handler->startGroup("INDEX_TREE_LAYOUT", layout[0] + 1);

            {
                Int value = layout[0];
                handler->value("LEVELS#", &value);
            }

            for (Int c = 1; c <= layout[0]; c++)
            {
                Int value = layout[c];
                handler->value("LVL_SIZE", &value);
            }

            handler->endGroup();
        }

        handler->startGroup("INDEXES", index_size());

        for (Int c = 0; c < index_size(); c++)
        {
            IndexValue indexes[Indexes];
            for (Int idx = 0; idx < Indexes; idx++)
            {
                indexes[idx] = this->indexes(idx)[c];
            }

            handler->value("INDEX", indexes, Indexes);
        }

        handler->endGroup();

        handler->startGroup("DATA", size());

        for (Int idx = 0; idx < meta->size() ; idx++)
        {
            Value values[Blocks];
            for (Int block = 0; block < Blocks; block++)
            {
                values[block] = this->getValue(block, idx);
            }

            handler->value("TREE_ITEM", values, Blocks);
        }

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        const Metadata* meta = this->metadata();

        FieldFactory<Int>::serialize(buf, meta->size());
        FieldFactory<Int>::serialize(buf, meta->data_size());
        FieldFactory<Int>::serialize(buf, meta->index_size());

        FieldFactory<OffsetsType>::serialize(buf, offsetsBlock(), offsetsBlockLength());

        if (has_index_layout())
        {
            auto layout = this->index_layout();
            FieldFactory<LayoutValue>::serialize(buf, layout, layout[0] + 1);
        }

        FieldFactory<IndexValue>::serialize(buf, indexes(0), Indexes * meta->index_size());

        FieldFactory<BufferType>::serialize(buf, values(), this->data_length());
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<Int>::deserialize(buf, meta->size());
        FieldFactory<Int>::deserialize(buf, meta->data_size());
        FieldFactory<Int>::deserialize(buf, meta->index_size());

        FieldFactory<OffsetsType>::deserialize(buf, offsetsBlock(), offsetsBlockLength());

        if (has_index_layout())
        {
            auto layout = this->index_layout();
            FieldFactory<LayoutValue>::deserialize(buf, layout, 1);
            FieldFactory<LayoutValue>::deserialize(buf, layout + 1, layout[0]);
        }

        FieldFactory<IndexValue>::deserialize(buf, indexes(0), Indexes * meta->index_size());

        FieldFactory<BufferType>::deserialize(buf, values(), this->data_length());
    }


    //=======================
    template <typename T>
    void _add(Int block, T& value) const
    {
    	value += sum(block);
    }

    template <typename T>
    void _add(Int block, Int end, T& value) const
    {
    	value += sum(block, end);
    }

    template <typename T>
    void _add(Int block, Int start, Int end, T& value) const
    {
    	value += sum(block, start, end);
    }



    template <typename T>
    void _sub(Int block, T& value) const
    {
    	value -= sum(block);
    }

    template <typename T>
    void _sub(Int block, Int end, T& value) const
    {
    	value -= sum(block, end);
    }

    template <typename T>
    void _sub(Int block, Int start, Int end, T& value) const
    {
    	value -= sum(block, start, end);
    }
private:
    static Int index_size(Int items_number)
    {
        if (items_number > ValuesPerBranch)
        {
            return TreeTools::compute_index_size(items_number);
        }
        else {
            return 0;
        }
    }


    ValueDescr raw_sum(Int to) const
    {
        GetVLEValuesSumFn<MyType> fn(*this, to);

        Int pos = TreeTools::find(fn);

        return ValueDescr(fn.value(), pos, to);
    }

    IndexValue raw_sum(Int from, Int to) const
    {
        return raw_sum(to).value() - raw_sum(from).value();
    }


    Int max_offset() const
    {
        return value_offset(size() * Blocks);
    }

    Int max_offset(Int block) const
    {
        return value_offset(size() * (block + 1));
    }

    Int data_length(Int block, Int start, Int end) const
    {
        Int size        = this->size();
        Int pos_start   = value_offset(size * block + start);
        Int pos_end     = value_offset(size * block + end);

        return pos_end - pos_start;
    }

    Int value_block_offset(Int block) const
    {
        GetValueOffsetFn fn(*this, block * size());

        Int pos = TreeTools::find_fw(fn);

        return pos;
    }

    static Int getValueBlocks(Int items_num)
    {
        return divUp(items_num, ValuesPerBranch);
    }

    Int getValueBlocks() const {
        return getValueBlocks(raw_capacity());
    }

    Int getOffsetsLengts() const
    {
        return getOffsetsBlockLength(raw_capacity());
    }

    static Int getOffsetsBlockLength(Int items_num)
    {
        Int value_blocks = getValueBlocks(items_num);

        Int offsets_bits = value_blocks * BITS_PER_OFFSET;

        Int offsets_bytes = Base::roundUpBitsToAlignmentBlocks(offsets_bits);

        return offsets_bytes;
    }

    Int offsetsBlockLength() const {
        return Base::element_size(OFFSETS) / sizeof(OffsetsType);
    }



    void enlarge(Int amount)
    {
        Int max_tree_capacity = max_capacity(raw_capacity() + amount);

        MEMORIA_ASSERT_TRUE(max_tree_capacity >= 0);

        Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        Int offsets_length  = Base::roundUpBytesToAlignmentBlocks(getOffsetsBlockLength(max_tree_capacity));

        Int layout_size     = MyType::index_layout_size(max_tree_capacity);
        Int layout_length   = Base::roundUpBytesToAlignmentBlocks(layout_size * sizeof(LayoutValue));

        Int index_size      = MyType::index_size(max_tree_capacity);
        Int index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * Indexes * sizeof(IndexValue));

        Int values_length   = Base::roundUpBitsToAlignmentBlocks(max_tree_capacity * Codec::ElementSize);

        Int block_size      = Base::block_size(
                metadata_length +
                offsets_length +
                layout_length +
                index_length +
                values_length,
                5
        );

        if (amount > 0)
        {
            resize(block_size);

            resizeBlock(OFFSETS, offsets_length);
            resizeBlock(LAYOUT,  layout_length);
            resizeBlock(INDEX,   index_length);
            resizeBlock(VALUES,  values_length);
        }
        else {
            resizeBlock(VALUES,  values_length);
            resizeBlock(INDEX,   index_length);
            resizeBlock(LAYOUT,  layout_length);
            resizeBlock(OFFSETS, offsets_length);

            resize(block_size);
        }

        TreeTools::buildIndexTreeLayout(this->index_layout(), max_tree_capacity, layout_size);

        Metadata* meta      = this->metadata();
        meta->index_size()  = index_size;

        reindex();
    }

    void shrink(Int amount)
    {
        enlarge(-amount);
    }

    void pack()
    {
        Int current_size        = this->data_size();
        Int required_capacity   = MyType::max_capacity(current_size);

        Int values_length       = this->element_size(VALUES);
        Int current_capacity    = values_length * 8 / Codec::ElementSize;

        Int capacity_delta      = current_capacity - required_capacity;

        shrink(capacity_delta);
    }

    void fillZero(Int start, Int end)
    {
        Codec codec;
        Value value = 0;
        auto values = this->values();

        for (Int idx = start; idx < end; idx++)
        {
            codec.encode(values, value, idx);
        }
    }


    void insertData(const Values* values, Int pos, Int processed)
    {
        Codec codec;

        Dimension total_lengths;

        for (Int block = 0; block < Blocks; block++)
        {
            for (SizeT c = 0; c < processed; c++)
            {
                total_lengths[block] += codec.length(values[c][block]);
            }
        }

        auto insertion_starts = insertSpace(pos, total_lengths);

        auto buffer = this->values();

        for (Int block = 0; block < Blocks; block++)
        {
            size_t start = insertion_starts[block];
            size_t limit = start + total_lengths[block];

            for (SizeT c = 0; c < processed; c++)
            {
                IndexValue value = values[c][block];
                Int len = codec.encode(buffer, value, start, limit);
                start += len;
            }
        }

        this->size() += processed;

        reindex();
    }

    void checkAndDump() {
        try {
            check();
        }
        catch (...) {
            dump();
            dumpData();
            dumpBitmap();
            throw;
        }
    }

    void readData(Values* values, Int pos, Int processed) const
    {
        Codec codec;

        const auto* buffer  = this->values();
        Int size            = this->size();

        for (Int block = 0; block < Blocks; block++)
        {
            Int value_idx = size * block + pos;

            size_t start = this->value_offset(value_idx);

            for (SizeT c = 0; c < processed; c++)
            {
                start += codec.decode(buffer, values[c][block], start);
            }
        }
    }

    template <Int Offset, typename Vals>
    void sumsSmall(Vals& values) const
    {
        Codec codec;

        auto buffer = this->values();

        Int size = this->size();

        for (Int block = 0, pos = 0; block < Blocks; block++)
        {
            Int block_end = this->value_offset((block + 1) * size);

            while (pos < block_end)
            {
                Value value;
                pos += codec.decode(buffer, value, pos, block_end);
                values[block + Offset] += value;
            }
        }
    }




};


template <typename Types>
struct PkdStructSizeType<PkdVTree<Types>> {
	static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


}


#endif
