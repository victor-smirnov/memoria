
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_TREE_HPP_
#define MEMORIA_CORE_PACKED_FSE_TREE_HPP_

#include <memoria/core/packed/tree/packed_tree_walkers.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/exint_codec.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/tools/accessors.hpp>

#include <memoria/core/tools/static_array.hpp>


#include <type_traits>

namespace memoria {


template <typename Value, typename Walker = EmptyType>
class FSEValueDescr {
    Value value_;
    Value prefix_;
    Int idx_;

    Walker walker_;
public:
    FSEValueDescr(BigInt value, Int idx, Value prefix, const Walker& walker = EmptyType()):
        value_(value),
        prefix_(prefix),
        idx_(idx),
        walker_(walker)
    {}

    Value value() const     {return value_;}
    Value prefix() const    {return prefix_;}
    Int idx() const         {return idx_;}

    const Walker& walker() const    {return walker_;}
    Walker& walker()                {return walker_;}
};



template <typename Types_>
class PkdFTree: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PkdFTree<Types>                                                     MyType;

    typedef typename Types::Allocator                                           Allocator;

    typedef typename Types::IndexValue                                          IndexValue;
    typedef typename Types::Value                                               Value;

    static const Int BranchingFactor        = Types::BranchingFactor;
    static const Int ValuesPerBranch        = Types::ValuesPerBranch;
    static const Int Indexes                = 1;
    static const Int Blocks                 = Types::Blocks;

    static const bool FixedSizeElement      = true;

    static const Int IOBatchSize            = Blocks <= 2 ? 256: (Blocks <= 4 ? 64 : (Blocks <= 16 ? 16 : 4));

    typedef core::StaticVector<IndexValue, Blocks>                              Values;
    typedef core::StaticVector<IndexValue, Blocks + 1>                          Values2;

    typedef Short                                                               LayoutValue;

    typedef PackedTreeTools<BranchingFactor, ValuesPerBranch, LayoutValue>      TreeTools;

    typedef typename Types::template Codec<Value>                               Codec;
    typedef typename Codec::BufferType                                          BufferType;

    typedef FSEValueDescr<IndexValue>                                           ValueDescr;

    template <typename TreeType, typename MyType>
    using FindLTFnBase = FSEFindElementFnBase<TreeType, PackedCompareLE, MyType>;

    template <typename TreeType, typename MyType>
    using FindLEFnBase = FSEFindElementFnBase<TreeType, PackedCompareLT, MyType>;

private:

    Int size_;
    Int index_size_;
    Int max_size_;

    UByte buffer_[];

public:

    typedef typename MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                decltype(size_),
                decltype(max_size_),
                decltype(index_size_),
                IndexValue,
                Value
    >::Result                                                                   FieldsList;



    PkdFTree() {}

    Int raw_size() const {return size_ * Blocks;}
    Int raw_capacity() const {return max_size_ * Blocks;}

    Int& size() {return size_;}
    const Int& size() const {return size_;}

    Int data_size() const {
        return raw_size() * sizeof (Value);
    }

    const Int& index_size() const {return index_size_;}

    Int content_size_from_start(Int block) const
    {
        IndexValue max = sum(block);
        return this->findGEForward(block, 0, max).idx() + 1;
    }

    Int block_size(const MyType* other) const
    {
        return block_size(size_ + other->size_);
    }

    Int block_size() const
    {
        return this->allocator()->element_size(this);
    }

    static Int empty_size()
    {
        return sizeof(MyType);
    }

    IndexValue* indexes(Int index_block)
    {
        return T2T<IndexValue*>(buffer_ + index_size_ * sizeof(IndexValue) * (index_block));
    }

    const IndexValue* indexes(Int index_block) const
    {
        return T2T<const IndexValue*>(buffer_ + index_size_ * sizeof(IndexValue) * (index_block));
    }

    IndexValue maxValue(Int index_block) const
    {
        return indexes(index_block)[0];
    }

    Value* getValues()
    {
        return buffer_ + index_size_ * sizeof(IndexValue) * Indexes;
    }

    const Value* getValues() const
    {
        return buffer_ + index_size_ * sizeof(IndexValue) * Indexes;
    }

    static Int getValueBlocks(Int items_num)
    {
        return items_num / ValuesPerBranch + (items_num % ValuesPerBranch ? 1 : 0);
    }

    Int getValueBlocks() const {
        return getValueBlocks(max_size_);
    }

    Int getOffsetsLengts() const
    {
        return getOffsetsLengts(max_size_);
    }

    static Int packed_block_size(Int tree_capacity)
    {
        return block_size(tree_capacity);
    }

    static Int block_size(Int tree_capacity)
    {
        Int index_size = MyType::index_size(tree_capacity);

        Int raw_block_size = sizeof(MyType) +
                                index_size *
                                Indexes *
                                sizeof(IndexValue) +
                                tree_capacity *
                                sizeof(Value) *
                                Blocks;

        return PackedAllocatable::roundUpBytesToAlignmentBlocks(raw_block_size);
    }

    static Int estimate_block_size(Int tree_capacity, Int density_hi = 1, Int density_lo = 1)
    {
        MEMORIA_ASSERT(density_hi, ==, 1); // data density should not be set for this type of trees
        MEMORIA_ASSERT(density_lo, ==, 1);

        return block_size(tree_capacity);
    }

    static Int elements_for(Int block_size)
    {
        return tree_size(block_size);
    }

    static Int expected_block_size(Int items_num)
    {
        return block_size(items_num);
    }

    static Int index_size(Int items_number)
    {
        if (items_number > ValuesPerBranch)
        {
            return TreeTools::compute_index_size(items_number * Blocks);
        }
        else {
            return 0;
        }
    }

    // ================================= Reindexing ======================================== //

private:

    class ReindexFn: public ReindexFnBase<MyType> {
        typedef ReindexFnBase<MyType> Base;
    public:
        ReindexFn(MyType& me): Base(me) {}

        void buildFirstIndexLine(Int index_level_start, Int index_level_size)
        {
            auto& me = Base::tree();
            auto& indexes = Base::indexes_;

            Int idx         = 0;
            Int block_start = 0;
            Int limit       = me.raw_size();
            auto values     = me.values();

            for (; idx < index_level_size; idx++, block_start += ValuesPerBranch)
            {
                Int next        = block_start + ValuesPerBranch;
                Int local_limit = next <= limit ? next : limit;

                IndexValue value = 0;

                for (Int c = block_start; c < local_limit; c++)
                {
                    value += values[c];
                }

                indexes[0][idx + index_level_start] = value;
            }
        }
    };


    class CheckFn: public CheckFnBase<MyType> {
        typedef CheckFnBase<MyType> Base;
    public:
        CheckFn(const MyType& me): Base(me) {}

        void buildFirstIndexLine(Int index_level_start, Int index_level_size) const
        {
            auto& me = Base::tree();
            auto& indexes = Base::indexes_;

            Int idx         = 0;
            Int block_start = 0;
            Int limit       = me.raw_size();
            auto values     = me.values();

            for (; idx < index_level_size; idx++, block_start += ValuesPerBranch)
            {
                Int next        = block_start + ValuesPerBranch;
                Int local_limit = next <= limit ? next : limit;

                IndexValue value = 0;

                for (Int c = block_start; c < local_limit; c++)
                {
                    value += values[c];
                }

                MEMORIA_ASSERT(indexes[0][idx + index_level_start], ==, value);
            }
        }

        void checkData() const {

        }
    };



public:

    void reindex()
    {
        ReindexFn fn(*this);
        TreeTools::reindex2(fn);
    }

    void check() const
    {
//      CheckFn fn(*this);
//      TreeTools::reindex2(fn);
    }


    // ==================================== Value ========================================== //

    Value& operator[](Int idx)
    {
        return value(idx);
    }

    const Value& operator[](Int idx) const
    {
        return value(idx);
    }

    void appendValue(Value value) {
        (*this)[size()++] = value;
        reindex();
    }

    Value& value(Int idx)
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <=, raw_size());

        return *(values() + idx);
    }



    const Value& value(Int idx) const
    {
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(idx, <, raw_size());

        return *(values() + idx);
    }


    Value& value(Int block, Int idx)
    {
        MEMORIA_ASSERT(idx, <=, size());
        return *(values(block) + idx);
    }

    const Value& value(Int block, Int idx) const
    {
        MEMORIA_ASSERT(idx, <, size());
        return *(values(block) + idx);
    }

    Int setValue(Int idx, Value val)
    {
        value(idx) = val;
        return 0;
    }

    void setValues(Int idx, const core::StaticVector<Value, Blocks>& values)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            value(block, idx) = values[block];
        }

        reindex();
    }

    void addValues(Int idx, const core::StaticVector<Value, Blocks>& values)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            value(block, idx) += values[block];
        }

        reindex();
    }

    void addValue(Int block, Int idx, Value value)
    {
        if (value != 0)
        {
            this->value(block, idx) += value;
        }

        reindex();
    }

    template <typename Value, Int Indexes>
    void addValues(Int idx, Int from, Int size, const core::StaticVector<Value, Indexes>& values)
    {
        for (Int block = 0; block < size; block++)
        {
            value(block, idx) += values[block + from];
        }

        reindex();
    }

    BigInt setValue(Int block, Int idx, Value value)
    {
        if (value != 0)
        {
            Value val = this->value(block, idx);
            this->value(block, idx) = value;

            return val - value;
        }
        else {
            return 0;
        }
    }

    Value* values()
    {
        return T2T<Value*>(buffer_ + index_size_ * sizeof(IndexValue) * Indexes);
    }

    const Value* values() const
    {
        return T2T<const Value*>(buffer_ + index_size_ * sizeof(IndexValue) * Indexes);
    }

    Value* values(Int block)
    {
        return values() + block * size_;
    }

    const Value* values(Int block) const
    {
        return values() + block * size_;
    }

    Value last_value(Int block) const
    {
        return value(block, size_ - 1);
    }

    Value first_value(Int block) const
    {
        return value(block, 0);
    }

    void clearIndex()
    {
        for (Int i = 0; i < Indexes; i++)
        {
            auto index = this->indexes(i);

            for (Int c = 0; c < index_size_; c++)
            {
                index[c] = 0;
            }
        }
    }

    void clear(Int start, Int end)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            Value* values = this->values(block);

            for (Int c = start; c < end; c++)
            {
                values[c] = 0;
            }
        }
    }

    // ==================================== Allocation =========================================== //

private:
    struct InitFn {
        Int block_size(Int items_number) const {
            return MyType::block_size(items_number);
        }

        Int max_elements(Int block_size)
        {
            return block_size;
        }
    };

public:
    static Int tree_size(Int block_size)
    {
        return block_size >= (Int)sizeof(Value) ? FindTotalElementsNumber2(block_size, InitFn()) : 0;
    }

    void init(Int block_size)
    {
        size_ = 0;

        max_size_   = tree_size(block_size);
        index_size_ = index_size(max_size_);
    }

    void inits(Int capacity)
    {
        size_ = 0;

        max_size_   = capacity;
        index_size_ = index_size(capacity);
    }

    void init()
    {
        size_ = 0;

        max_size_   = 0;
        index_size_ = index_size(max_size_);
    }

    void clear()
    {
        inits(0);

        if (Base::has_allocator())
        {
            Allocator* alloc = this->allocator();
            Int empty_size = MyType::empty_size();
            alloc->resizeBlock(this, empty_size);
        }
    }

    Int object_size() const
    {
        Int object_size = sizeof(MyType) + getDataOffset() + tree_data_size();
        return PackedAllocatable::roundUpBytesToAlignmentBlocks(object_size);
    }

    Int tree_data_size() const
    {
        return size() * sizeof(Value) * Blocks;
    }

    Int getTotalDataSize() const
    {
        return max_size() * sizeof(Value) * Blocks;
    }

    Int getDataOffset() const
    {
        return index_size_ * sizeof(IndexValue) * Indexes;
    }

    Int capacity() const {
        return max_size_ - size_;
    }

    Int total_capacity() const
    {
        Int my_size     = allocator()->element_size(this);
        Int free_space  = allocator()->free_space();
        Int total_size  = MyType::tree_size(my_size + free_space);
        Int capacity    = total_size - size_;

        return capacity >= 0 ? capacity : 0;
    }

    void transferTo(MyType* other, Value* target_memory_block = nullptr) const
    {
        if (target_memory_block == nullptr)
        {
            target_memory_block = other->values();
        }

        Int data_size = this->tree_data_size();

        const Value* data = values();

        CopyByteBuffer(data, target_memory_block, data_size);
    }





    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout, bool dump_index = true) const
    {
        out<<"size_       = "<<size_<<endl;
        out<<"max_size_   = "<<max_size_<<endl;

        if (dump_index)
        {
            out<<"index_size_ = "<<index_size_<<endl;
            out<<"Indexes     = "<<Indexes<<endl;
            out<<endl;

            Int idx_max = index_size_;

            out<<"Indexes:"<<endl;

            for (Int c = 0; c < idx_max; c++)
            {
                out<<c<<" ";

                for (Int idx = 0; idx < Indexes; idx++)
                {
                    out<<this->indexes(idx)[c]<<" ";
                }

                out<<endl;
            }
        }

        out<<endl;

        out<<"Data:"<<endl;

        for (Int c = 0; c < size_; c++)
        {
            out<<c<<" ";

            for (Int block = 0; block < Blocks; block++)
            {
                out<<value(block, c)<<" ";
            }

            out<<endl;
        }
    }

    // ==================================== Query ========================================== //

    IndexValue sum(Int block) const
    {
        return sum(block, size_);
    }

    IndexValue sum(Int block, Int to) const
    {
        Int base = block * size_;
        return raw_sum(base, base + to);
    }


    IndexValue sumWithoutLastElement(Int block) const
    {
        return sum(block, size_ - 1);
    }


    IndexValue sum(Int block, Int from, Int to) const
    {
        Int base = block * size_;
        return raw_sum(base + to) - raw_sum(base + from);
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

        for (Int block = 0; block < Blocks; block++)
        {
            vals[block] = sum(block);
        }

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
        values += sums();
    }

    void sums(Values2& values) const
    {
        values[0] += size();
        values.sumUp(sums());
    }

    void sums(Int idx, Values2& values) const
    {
        addKeys(idx, values);
    }

    void sums(Int idx, Values& values) const
    {
        addKeys(idx, values);
    }

    void addKeys(Int idx, Values& values) const
    {
        for (Int block = 0; block < Blocks; block++)
        {
            values[block] += this->value(block, idx);
        }
    }

    void addKeys(Int idx, Values2& values) const
    {
        values[0] += 1;

        for (Int block = 0; block < Blocks; block++)
        {
            values[block + 1] += this->value(block, idx);
        }
    }


    ValueDescr findGTForward(Int block, Int start, IndexValue val) const
    {
        Int block_start = block * size_;

        auto prefix = raw_sum(block_start + start);

        FSEFindElementFn<MyType, PackedCompareLE> fn(*this, val + prefix);

        Int pos = TreeTools::find2(fn);

        if (pos < block_start + size_)
        {
            Value actual_value = value(pos);

            return ValueDescr(actual_value, pos - block_start, fn.sum() - prefix);
        }
        else {
            return ValueDescr(0, size_, sum(block) - prefix);
        }
    }



    ValueDescr findGTBackward(Int block, Int start, IndexValue val) const
    {
        Int block_start = block * size_;

        auto prefix = raw_sum(block_start + start + 1);
        auto target = prefix - val;

        if (target > 0)
        {
            FSEFindElementFn<MyType, PackedCompareLT> fn(*this, target);

            Int pos = TreeTools::find2(fn);

            if (pos > block_start + start)
            {
                return ValueDescr(0, start, 0);
            }
            else if (pos >= block_start)
            {
                Value actual_value = value(pos);
                return ValueDescr(actual_value, pos - block_start, prefix - (fn.sum() + actual_value));
            }
            else {
                return ValueDescr(0, -1, prefix - raw_sum(block_start));
            }
        }
        else {
            return ValueDescr(0, -1, prefix);
        }
    }



    ValueDescr findGEForward(Int block, Int start, IndexValue val) const
    {
        Int block_start = block * size_;

        auto prefix = raw_sum(block_start + start);

        FSEFindElementFn<MyType, PackedCompareLT> fn(*this, val + prefix);

        Int pos = TreeTools::find2(fn);

        if (pos < block_start + size_)
        {
            Value actual_value = value(pos);

            return ValueDescr(actual_value, pos - block_start, fn.sum() - prefix);
        }
        else {
            return ValueDescr(0, size_, sum(block, start, size_));
        }
    }

    ValueDescr findGEBackward(Int block, Int start, IndexValue val) const
    {
        Int block_start = block * size_;

        if (val > 0)
        {
            auto prefix = raw_sum(block_start + start + 1);
            auto target = prefix - val;

            if (target > 0)
            {
                FSEFindElementFn<MyType, PackedCompareLE> fn(*this, target);

                Int pos = TreeTools::find2(fn);

                if (pos >= block_start)
                {
                    Value actual_value = value(pos);
                    return ValueDescr(actual_value, pos - block_start, prefix - (fn.sum() + actual_value));
                }
                else {
                    return ValueDescr(0, -1, prefix - raw_sum(block_start));
                }
            }
            else if (target == 0)
            {
                Value actual_value = value(block_start);
                return ValueDescr(actual_value, 0, prefix - raw_sum(block_start) - actual_value);
            }
            else {
                return ValueDescr(0, -1, prefix);
            }
        }
        else {
            Value actual_value = value(start);
            return ValueDescr(actual_value, start, 0);
        }
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



    // ==================================== Update ========================================== //


    bool ensureCapacity(Int size)
    {
        Int capacity = this->capacity();
        if (capacity < size)
        {
            enlarge(size - capacity);
            return true;
        }
        else {
            return false;
        }
    }


    void insert(Int idx, Int size, std::function<Values ()> provider)
    {
        insertSpace(idx, size);

        Int my_size = this->size();

        Value* values = this->values();

        for (Int c = idx; c < idx + size; c++)
        {
            Values vals = provider();

            for (Int block = 0; block < Blocks; block++)
            {
                values[block * my_size + c] = vals[block];
            }
        }

        reindex();
    }

    void update(Int start, Int end, std::function<Values ()> provider)
    {
    	Int my_size = this->size();

    	MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, end);

        Value* values = this->values();

        for (Int c = start; c < end; c++)
        {
            Values vals = provider();

            for (Int block = 0; block < Blocks; block++)
            {
                values[block * my_size + c] = vals[block];
            }
        }

        reindex();
    }

    void read(Int start, Int end, std::function<void (const Values&)> consumer) const
    {
    	Int my_size = this->size();

    	MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, end);
        MEMORIA_ASSERT(end, <=, my_size);

        const Value* values = this->values();

        for (Int c = start; c < end; c++)
        {
            Values vals;

            for (Int block = 0; block < Blocks; block++)
            {
                vals[block] = values[block * my_size + c];
            }

            consumer(vals);
        }
    }


    Int insert(Int idx, std::function<bool (Values&)> provider)
    {
        Values vals;
        Int cnt = 0;

        while(provider(vals) && check_capacity(1))
        {
            insertSpace(idx, 1);

            Value* values   = this->values();
            Int my_size     = this->size();

            for (Int block = 0; block < Blocks; block++)
            {
                values[block * my_size + idx] = vals[block];
            }

            idx++;
            cnt++;
        }

        reindex();

        return cnt;
    }

    void insert(Int idx, const Values& values)
    {
        insertSpace(idx, 1);
        setValues(idx, values);
    }

    void insertSpace(Int idx, Int room_length)
    {
        MEMORIA_ASSERT(idx, <=, this->size());
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(room_length, >=, 0);

        Int capacity = this->capacity();

        if (capacity < room_length)
        {
            enlarge(room_length - capacity);
        }

        Value* values = this->values();

        for (Int block = 0; block < Blocks; block++)
        {
            Int offset = (Blocks - block - 1) * size_ + idx;

            CopyBuffer(
                    values + offset,
                    values + offset + room_length,
                    size_ * Blocks - offset + room_length * block
            );
        }

        size_ += room_length;

        clear(idx, idx + room_length);

        MEMORIA_ASSERT(raw_size(), <=, raw_capacity());
    }

    void removeSpace(Int start, Int end)
    {
        remove(start, end);
    }

    void remove(Int start, Int end)
    {
        MEMORIA_ASSERT_TRUE(start >= 0);
        MEMORIA_ASSERT_TRUE(end >= 0);

        Int room_length = end - start;

        MEMORIA_ASSERT(room_length, <= , size() - start);

        Value* values = this->values();

        for (Int block = Blocks - 1, sum = 0; block >= 0; block--)
        {
            Int to      = block * size_ + start;
            Int from    = block * size_ + end;

            Int length = size_ - end + sum;

            CopyBuffer(
                    values + from,
                    values + to,
                    length
            );

            sum += size_ - (end - start);
        }

        size_ -= room_length;

        shrink(room_length);
    }

    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
        MEMORIA_ASSERT_TRUE(copy_from >= 0);
        MEMORIA_ASSERT_TRUE(count >= 0);

        for (Int block = 0; block < Blocks; block++)
        {
            Int my_offset       = block * size_;
            Int other_offset    = block * other->size_;

            CopyBuffer(
                    this->values() + copy_from + my_offset,
                    other->values() + copy_to + other_offset,
                    count
            );
        }
    }

    void splitTo(MyType* other, Int idx)
    {
        Int total = this->size() - idx;
        other->insertSpace(0, total);

        copyTo(other, idx, total, 0);
        other->reindex();

        removeSpace(idx, this->size());
        reindex();
    }

    void mergeWith(MyType* other)
    {
        Int my_size     = this->size();
        Int other_size  = other->size();

        other->insertSpace(other_size, my_size);

        copyTo(other, 0, my_size, other_size);

        removeSpace(0, my_size);

        reindex();
        other->reindex();
    }

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        other->insertSpace(0, size());

        const auto* my_values   = values();
        auto* other_values      = other->values();

        Int size = this->size();

        for (Int c = 0; c < size * Blocks; c++)
        {
            other_values[c]     = my_values[c];
        }
    }

    void resize(Int delta)
    {
        if (delta > 0)
        {
            insertSpace(size_, delta);
        }
        else {
            removeSpace(size_, -delta);
        }
    }


    // ===================================== IO ============================================ //

    static Int computeDataLength(const Values& values)
    {
    	return Blocks;
    }


    void insert(IData* data, Int pos, Int length)
    {
        insertSpace(pos, length);
        update(data, pos, length);
    }

    void append(IData* data, Int length)
    {
        if (capacity() < length)
        {
            Allocator* alloc = this->allocator();

            Int block_size = alloc->element_size(this) + alloc->free_space();

            Int size = MyType::elements_for(block_size);

            if (length > size)
            {
                length = size;
            }
        }

        insert(data, size(), length);
    }


    void update(IData* data, Int pos, Int length)
    {
        MEMORIA_ASSERT(pos, <=, size_);
        MEMORIA_ASSERT(pos + length, <=, size_);

        IDataSource<Values>* src = static_cast<IDataSource<Values>*>(data);

        IDataAPI api_type = src->api();

        if (api_type == IDataAPI::Batch || api_type == IDataAPI::Both)
        {
            Values values[IOBatchSize];

            BigInt to_write_local = length;

            while (to_write_local > 0)
            {
                SizeT remainder     = src->getRemainder();
                SizeT batch_size    = remainder > IOBatchSize ? IOBatchSize : remainder;

                SizeT processed = src->get(&values[0], 0, batch_size);

                for (Int b = 0; b < Blocks; b++)
                {
                    Value* vals = this->values(b);

                    for (SizeT c = 0; c < processed; c++)
                    {
                        vals[pos + c] = values[c][b];
                    }
                }

                pos             += processed;
                to_write_local  -= processed;
            }
        }
        else {
            for (Int c = 0; c < length; c++)
            {
                Values v = src->get();

                for (Int b = 0; b < Blocks; b++)
                {
                    this->value(b, pos + c) = v[b];
                }
            }
        }

        reindex();
    }

    void read(IData* data, Int pos, Int length) const
    {
        MEMORIA_ASSERT(pos, <=, size_);
        MEMORIA_ASSERT(pos + length, <=, size_);

        IDataTarget<Value>* tgt = static_cast<IDataTarget<Value>*>(data);

        IDataAPI api_type = tgt->api();

        if (api_type == IDataAPI::Batch || api_type == IDataAPI::Both)
        {
            Values values[IOBatchSize];

            BigInt to_read_local = length;

            while (to_read_local > 0)
            {
                SizeT processed = tgt->put(&values[0], 0, tgt->getRemainder());

                for (Int b = 0; b < Blocks; b++)
                {
                    const Value* vals = this->values(b);

                    for (SizeT c = 0; c < processed; c++)
                    {
                        values[c][b] = vals[pos + c];
                    }
                }

                pos             += processed;
                to_read_local   -= processed;
            }
        }
        else {
            for (Int c = 0; c < length; c++)
            {
                Values v;

                for (Int b = 0; b < Blocks; b++)
                {
                    v[b] = this->value(b, pos + c);
                }

                tgt->put(v);
            }
        }
    }



    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("FSE_TREE");

        handler->value("ALLOCATOR",     &Base::allocator_offset());
        handler->value("SIZE",          &size_);
        handler->value("MAX_SIZE",      &max_size_);
        handler->value("INDEX_SIZE",    &index_size_);

        handler->startGroup("INDEXES", index_size_);

        for (Int c = 0; c < index_size_; c++)
        {
            IndexValue indexes[Indexes];
            for (Int idx = 0; idx < Indexes; idx++)
            {
                indexes[idx] = this->indexes(idx)[c];
            }

            handler->value("INDEX", indexes, Indexes);
        }

        handler->endGroup();

        handler->startGroup("DATA", size_);

        for (Int idx = 0; idx < size_ ; idx++)
        {
            Value values[Blocks];
            for (Int block = 0; block < Blocks; block++)
            {
                values[block] = value(block, idx);
            }

            handler->value("TREE_ITEM", values, Blocks);
        }

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        FieldFactory<Int>::serialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::serialize(buf, size_);
        FieldFactory<Int>::serialize(buf, max_size_);
        FieldFactory<Int>::serialize(buf, index_size_);

        FieldFactory<IndexValue>::serialize(buf, indexes(0), Indexes * index_size());

        FieldFactory<Value>::serialize(buf, values(), size_ * Blocks);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::deserialize(buf, size_);
        FieldFactory<Int>::deserialize(buf, max_size_);
        FieldFactory<Int>::deserialize(buf, index_size_);

        FieldFactory<IndexValue>::deserialize(buf, indexes(0), Indexes * index_size());

        FieldFactory<Value>::deserialize(buf, values(), size_ * Blocks);
    }


    void updateUp(Int block_num, Int idx, IndexValue key_value)
    {
        value(block_num, idx) += key_value;
    }


private:
    const Int& max_size() const {return max_size_;}


    IndexValue raw_sum(Int to) const
    {
        if (index_size_ == 0 || to < ValuesPerBranch)
        {
            IndexValue sum = 0;

            const Value* values = this->values();

            for (Int c = 0; c < to; c++)
            {
                sum += values[c];
            }

            return sum;
        }
        else {
            GetFSEValuesSumFn<MyType> fn(*this);

            TreeTools::walk2(to, fn);

            return fn.sum();
        }
    }

    IndexValue raw_sum(Int from, Int to) const
    {
        return raw_sum(to) - raw_sum(from);
    }

    bool check_capacity(Int size) const
    {
        MEMORIA_ASSERT_TRUE(size >= 0);

        auto alloc = this->allocator();

        Int total_size          = this->size() + size;
        Int total_block_size    = MyType::block_size(total_size);
        Int my_block_size       = alloc->element_size(this);
        Int delta               = total_block_size - my_block_size;

        return alloc->free_space() >= delta;
    }

    void enlarge(Int items_num)
    {
        Allocator* alloc = allocator();

        MyType other;

        Int requested_block_size = MyType::block_size(max_size_ + items_num);

        Int new_size = alloc->resizeBlock(this, requested_block_size);

        other.init(new_size);
        other.size()                = this->size();
        other.allocator_offset()    = this->allocator_offset();

        MEMORIA_ASSERT(other.size(), <=, other.max_size());
        MEMORIA_ASSERT(other.capacity(), >=, items_num);

        transferTo(&other, T2T<Value*>(buffer_ + other.getDataOffset()));

        *this = other;
    }

    void shrink(Int items_num)
    {
        MEMORIA_ASSERT(items_num, <=, max_size_);
        MEMORIA_ASSERT(max_size_ - items_num, >=, size_);

        Allocator* alloc = allocator();

        MyType other;

        Int requested_block_size = MyType::block_size(max_size_ - items_num);
        Int current_block_size   = alloc->element_size(this);

        if (requested_block_size < current_block_size)
        {
            other.init(requested_block_size);
            other.size()                = this->size();
            other.allocator_offset()    = this->allocator_offset();

            MEMORIA_ASSERT(other.size(), <=, other.max_size());

            transferTo(&other, T2T<Value*>(buffer_ + other.getDataOffset()));

            *this = other;

            alloc->resizeBlock(this, requested_block_size);
        }
    }
};





}


#endif
