
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_SIZED_STRUCT_HPP_
#define MEMORIA_CORE_PACKED_SIZED_STRUCT_HPP_

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/buffer/packed_fse_input_buffer_ro.hpp>
#include <memoria/core/tools/accessors.hpp>

namespace memoria {



class PackedSizedStruct: public PackedAllocatable {

    typedef PackedAllocatable                                                   Base;

public:
    static const UInt VERSION = 1;
    static constexpr Int Indexes = 0;
    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;


    using MyType = PackedSizedStruct;


    using Value = Int;

    static constexpr Int Blocks = 0;

    using InputType = Value;

    using InputBuffer = MyType;

    using Values = Value;
    using IndexValue = Value;
    using SizesT = core::StaticVector<Int, Blocks>;

private:

    Int size_;

public:
    PackedSizedStruct() {}

    Int& size() {return size_;}
    const Int& size() const {return size_;}


    Int block_size() const
    {
        return sizeof(MyType);
    }

    Int block_size(const MyType* other) const
    {
        return block_size(size_ + other->size_);
    }

    static constexpr Int block_size(Int array_size)
    {
        return sizeof(MyType);
    }

    static constexpr Int packed_block_size(Int array_size)
    {
        return sizeof(MyType);
    }

    void init(Int block_size)
    {
        size_ = 0;
    }

    void init(const SizesT& capacities)
    {
        size_ = 0;
    }

    static constexpr Int empty_size()
    {
        return sizeof(MyType);
    }


    void init()
    {
        size_ = 0;
    }



    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int start, Int end, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sub(Int start, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int idx, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");
    }


    // =================================== Update ========================================== //

    void reindex() {}
    void check() const {}

    void remove(Int start, Int end)
    {
    	MEMORIA_ASSERT_TRUE(start >= 0);
    	MEMORIA_ASSERT_TRUE(end >= 0);

    	Int room_length = end - start;

    	size_ -= room_length;
    }

    void removeSpace(Int room_start, Int room_end) {
        remove(room_start, room_end);
    }

    void insertSpace(Int idx, Int room_length)
    {
        MEMORIA_ASSERT(idx, <=, this->size());
        MEMORIA_ASSERT(idx, >=, 0);
        MEMORIA_ASSERT(room_length, >=, 0);

        size_ += room_length;
    }

    void reset()
    {
    	size_ = 0;
    }


    void splitTo(MyType* other, Int idx)
    {
        MEMORIA_ASSERT(other->size(), ==, 0);

        Int split_size = this->size() - idx;
        other->insertSpace(0, split_size);

        removeSpace(idx, this->size());
    }

    void mergeWith(MyType* other)
    {
        Int my_size     = this->size();
        Int other_size  = other->size();

        other->insertSpace(other_size, my_size);

        removeSpace(0, my_size);
    }

    // ===================================== IO ============================================ //

    void insert(Int pos, Value val)
    {
    	insertSpace(pos, 1);
    }

    void insert(Int block, Int pos, Value val)
    {
        insertSpace(pos, 1);
    }

    void insert(Int pos, Int start, Int size, const InputBuffer* buffer)
    {
    	insertSpace(pos, size);
    }

    template <typename Adaptor>
    void insert(Int pos, Int size, Adaptor&& adaptor)
    {
    	insertSpace(pos, size);
    }


    SizesT insert_buffer(SizesT at, const InputBuffer* buffer, SizesT starts, SizesT ends, Int size)
    {
    	insertSpace(at[0], size);
    	return at + SizesT(size);
    }

    Int insert_buffer(Int at, const InputBuffer* buffer, Int start, Int size)
    {
    	insertSpace(at, size);
    	return at + size;
    }

    SizesT positions(Int idx) const {
    	return SizesT(idx);
    }


    template <typename Adaptor>
    void _insert(Int pos, Int size, Adaptor&& adaptor)
    {
    	insertSpace(pos, size);
    }


    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class AccumItem>
    void _update(Int pos, Value&& val, AccumItem<T, Size>& accum)
    {}

    template <Int Offset, typename Value, typename T, Int Size, template <typename, Int> class AccumItem>
    void _insert(Int pos, Value&& val, AccumItem<T, Size>& accum)
    {
    	_insert(pos, 1, [&](int block, int idx){
    		return Value();
    	});
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void _remove(Int idx, AccumItem<T, Size>& accum)
    {
    	remove(idx, idx + 1);
    }


    template <typename Fn>
    void read(Int block, Int start, Int end, Fn&& fn) const
    {
        MEMORIA_ASSERT(start, <, size_);
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, >=, 0);
        MEMORIA_ASSERT(end, <=, size_);

        for (Int c = start; c < end; c++)
        {
        	fn(Value());
        }
    }

    template <typename Fn>
    void read(Int start, Int end, Fn&& fn) const
    {
        scan(start, end, std::forward<Fn>(fn));
    }



    template <typename Fn>
    SizesT scan(Int start, Int end, Fn&& fn) const
    {
        MEMORIA_ASSERT(start, <=, size_);
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(end, >=, 0);
        MEMORIA_ASSERT(end, <=, size_);

        for (Int c = start; c < end; c++)
        {
        	fn(Values());
        }

        return SizesT(end);
    }


    // ==================================== Dump =========================================== //


    void dump(std::ostream& out = cout) const
    {
        out<<"size_ = "<<size_<<endl;
    }


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	handler->startStruct();
        handler->startGroup("SIZED_STRUCT");

        handler->value("ALLOCATOR",     &Base::allocator_offset());
        handler->value("SIZE",          &size_);

        handler->endGroup();
    	handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
    	FieldFactory<Int>::serialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::serialize(buf, size_);
    }

    void deserialize(DeserializationData& buf)
    {
        FieldFactory<Int>::deserialize(buf, Base::allocator_offset_);
        FieldFactory<Int>::deserialize(buf, size_);
    }
};


template <>
struct PkdStructSizeType<PackedSizedStruct> {
	static const PackedSizeType Value = PackedSizeType::FIXED;
};

template <>
struct StructSizeProvider<PackedSizedStruct> {
    static const Int Value = 0;
};



}


#endif
