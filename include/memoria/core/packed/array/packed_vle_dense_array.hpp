
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_DENSE_ARRAY_HPP_
#define MEMORIA_CORE_PACKED_VLE_DENSE_ARRAY_HPP_

#include <memoria/core/packed/array/packed_vle_array_base.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_tools.hpp>
#include <memoria/core/packed/buffer/packed_vle_input_buffer_ro.hpp>

namespace memoria {



template <
	typename ValueT,
	Int kBlocks,
	template <typename> class CodecT,
	Int kBranchingFactor = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
	Int kValuesPerBranch = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
struct PkdVLEArrayTypes {
	using Value = ValueT;

	template <typename T>
	using Codec = CodecT<T>;

	static constexpr Int Blocks = kBlocks;
	static constexpr Int BranchingFactor = kBranchingFactor;
	static constexpr Int ValuesPerBranch = kValuesPerBranch;
};



template <typename Types> class PkdVDArray;

template <
	typename ValueT,
	Int kBlocks,
	template <typename> class CodecT,

	Int kBranchingFactor = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
	Int kValuesPerBranch = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
using PkdVDArrayT = PkdVDArray<PkdVLEArrayTypes<ValueT, kBlocks, CodecT, kBranchingFactor, kValuesPerBranch>>;


template <typename Types>
class PkdVDArray: public PkdVLEArrayBase<1, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch> {

	using Base 		= PkdVLEArrayBase<1, typename Types::Value, Types::template Codec, Types::BranchingFactor, Types::ValuesPerBranch>;
	using MyType 	= PkdVDArray<Types>;

public:
    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;

    using Base::metadata;
    using Base::locate;
    using Base::offsets_segment_size;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;



    using typename Base::Codec;

    static constexpr UInt VERSION = 1;
    static constexpr Int TreeBlocks = 1;
    static constexpr Int Blocks = Types::Blocks;

    static constexpr Int SafetyMargin = 128 / Codec::ElementSize;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<UInt, TreeBlocks>,
    			ConstValue<UInt, Blocks>
    >;

    using Value		 = typename Types::Value;

    using Values = core::StaticVector<Value, Blocks>;


    using InputBuffer 	= PkdVLERowOrderInputBuffer<Types>;
    using InputType 	= Values;


    using SizesT = core::StaticVector<Int, Blocks>;

    static Int estimate_block_size(Int tree_capacity, Int density_hi = 1000, Int density_lo = 333)
    {
        Int max_tree_capacity = (tree_capacity * Blocks * density_hi) / density_lo;
        return block_size(max_tree_capacity);
    }



    void init_tl(Int data_block_size)
    {
    	Base::init_tl(data_block_size, TreeBlocks);
    }

    void init(const SizesT& sizes) {
    	MyType::init(sizes.sum());
    }

    void init(Int total_capacity)
    {
    	Base::init(empty_size(), TreeBlocks * SegmentsPerBlock + BlocksStart);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);

    	meta->size() = 0;

    	for (Int block = 0; block < TreeBlocks; block++)
    	{
    		Int capacity        = total_capacity;
    		Int offsets_size 	= offsets_segment_size(capacity);
    		Int index_size		= this->index_size(capacity);
    		Int values_segment_length = this->value_segment_size(capacity);

    		this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
    		this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
    		this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
    	}
    }

    void init()
    {
    	Base::init(empty_size(), TreeBlocks * SegmentsPerBlock + BlocksStart);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);

    	meta->size() = 0;
    	Int offsets_size = offsets_segment_size(0);

    	for (Int block = 0; block < TreeBlocks; block++)
    	{
    		this->template allocateArrayBySize<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0);
    		this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
    		this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + VALUES + BlocksStart, 0);
    	}
    }


    static Int block_size(Int capacity)
    {
    	return Base::block_size_equi(TreeBlocks, capacity * Blocks);
    }

    static Int block_size(const SizesT& capacity)
    {
    	return Base::block_size_equi(TreeBlocks, capacity.sum());
    }


    static Int packed_block_size(Int tree_capacity)
    {
    	return block_size(tree_capacity);
    }


    Int block_size() const
    {
    	return Base::block_size();
    }

    Int block_size(const MyType* other) const
    {
        return block_size(this->data_size() + other->data_size());
    }

    ValueData* values() {
    	return Base::values(0);
    }

    const ValueData* values() const {
    	return Base::values(0);
    }

    const Int& data_size() const {
    	return Base::data_size(0);
    }

    Int& data_size() {
    	return Base::data_size(0);
    }

    Int size() const
    {
    	return this->metadata()->size() / Blocks;
    }

//    SizesT data_size_v() const
//    {
//    	SizesT sizes;
//
//    	for (Int block = 0; block < Blocks; block++)
//    	{
//    		sizes[block] = this->data_size(block);
//    	}
//
//    	return sizes;
//    }

    static Int elements_for(Int block_size)
    {
        return Base::tree_size(TreeBlocks, block_size);
    }

    static Int expected_block_size(Int items_num)
    {
        return block_size(items_num);
    }

    Int locate(Int block, Int idx) const
    {
    	auto values = this->values();
    	auto data_size = this->data_size();

    	TreeLayout layout = Base::compute_tree_layout(data_size);

    	return locate(layout, values, block, idx * Blocks + block, data_size).idx;
    }


    Value value(Int block, Int idx) const
    {
    	Int size = this->size();

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <, size);

    	Int data_size	  = this->data_size();
    	auto values 	  = this->values();
    	TreeLayout layout = this->compute_tree_layout(data_size);

    	Int global_idx    = idx * Blocks + block;
		Int start_pos  	  = this->locate(layout, values, 0, global_idx, data_size).idx;

		MEMORIA_ASSERT(start_pos, <, data_size);

		Codec codec;
		Value value;

		codec.decode(values, value, start_pos);

		return value;
    }

    static Int empty_size()
    {
    	return block_size(0);
    }

    void reindex() {
    	Base::reindex(TreeBlocks);
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


    // ================================ Container API =========================================== //



    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int start, Int end, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sub(Int start, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int idx, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size, "Invalid balanced tree structure");
    }

    Values get_values(Int idx) const
    {
    	Int size = this->size();

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <, size);

    	Int data_size	  = this->data_size();
    	auto values 	  = this->values();
    	TreeLayout layout = this->compute_tree_layout(data_size);

    	Int global_idx 	  = idx * size;
		Int start_pos  	  = this->locate(layout, values, 0, global_idx).idx;

		MEMORIA_ASSERT(start_pos, <, data_size);

		Codec codec;
		Values values_data;

		for (Int b = 0; b < Blocks; b++)
		{
			auto len = codec.decode(values, values_data[b], start_pos);
			start_pos += len;
		}

		return values_data;
    }

    Value get_values(Int idx, Int index) const
    {
    	return this->value(index, idx);
    }

    Value getValue(Int index, Int idx) const
    {
    	return this->value(index, idx);
    }





    // ========================================= Insert/Remove/Resize ============================================== //

protected:
    void resize_segments(Int new_data_size)
    {
    	Int block = 0;

    	Int data_segment_size 	 = PackedAllocator::roundUpBitsToAlignmentBlocks(new_data_size * Codec::ElementSize);
    	Int index_size 	 	   	 = Base::index_size(new_data_size);
    	Int offsets_segment_size = Base::offsets_segment_size(new_data_size);

    	this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, data_segment_size);
    	this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_segment_size);
    	this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
    }

    auto insert_space(Int start, Int length)
    {
    	Int& data_size = this->data_size();
    	resize_segments(data_size + length);

    	Codec codec;
    	codec.move(this->values(), start, start + length, data_size - start);

    	data_size += length;
    }

    auto remove_space(Int start, Int length)
    {
    	Int& data_size = this->data_size();

    	Codec codec;
    	codec.move(this->values(), start + length, start, data_size - (start + length));

    	resize_segments(data_size - length);

    	data_size -= length;
    }


public:
    void splitTo(MyType* other, Int idx)
    {
    	Int size = this->size();
    	Int other_size = other->size();

    	Int other_lengths[Blocks];

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int start = other->locate(0, block * other_size + 0);
    		Int end = other->locate(0, block * other_size + other_size);

    		other_lengths[block] = end - start;
    	}

    	Codec codec;
    	Int insertion_pos = 0;
    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int start = this->locate(0, block * size + idx);
    		Int end   = this->locate(0, block * size + size);

    		Int length = end - start;

    		other->insert_space(insertion_pos, length);
    		codec.copy(this->values(), start, other->values(), insertion_pos, length);

    		insertion_pos += length + other_lengths[block];
    	}

    	other->metadata()->size() += (size - idx) * Blocks;

        other->reindex();

        remove(idx, size);
    }


    void mergeWith(MyType* other)
    {
    	Int size = this->size();
    	Int other_size = other->size();

    	Int other_lengths[Blocks];

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int start = other->locate(0, block * other_size + 0);
    		Int end = other->locate(0, block * other_size + other_size);

    		other_lengths[block] = end - start;
    	}

    	Codec codec;
    	Int insertion_pos = 0;
    	for (Int block = 0; block < Blocks; block++)
    	{
    		insertion_pos += other_lengths[block];

    		Int start = this->locate(0, block * size);
    		Int end   = this->locate(0, block * size + size);

    		Int length = end - start;

    		other->insert_space(insertion_pos, length);
    		codec.copy(this->values(), start, other->values(), insertion_pos, length);

    		insertion_pos += length;
    	}

    	other->metadata()->size() += size * Blocks;

    	other->reindex();

    	this->clear();
    }


    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
    	Codec codec;

    	Int data_size = this->data_size();
    	other->insertSpace(0, data_size);
    	codec.copy(this->values(), 0, other->values(), 0, data_size);

    	other->reindex();
    }


    void removeSpace(Int start, Int end) {
    	remove(start, end);
    }

    void remove(Int start, Int end)
    {
    	if (end > start)
    	{
    		Int& data_size		= this->data_size();
    		auto values			= this->values();
    		TreeLayout layout 	= compute_tree_layout(data_size);
    		Int size			= this->size();

    		Codec codec;

    		Int start_pos[Blocks];
    		Int lengths[Blocks];

    		for (Int block = 0; block < Blocks; block++)
    		{
    			start_pos[block] = this->locate(layout, values, 0, start + size * block, data_size).idx;
    			Int end_pos 	 = this->locate(layout, values, 0, end + size * block, data_size).idx;

    			lengths[block] = end_pos - start_pos[block];
    		}

    		Int total_length = 0;

    		for (Int block = Blocks - 1; block >= 0; block--)
    		{
    			Int length = lengths[block];
    			Int end = start_pos[block] + length;
    			Int start = start_pos[block];

    			codec.move(values, end, start, data_size - end);

    			total_length += length;
    			data_size -= length;
    		}

    		resize_segments(data_size);

    		metadata()->size() -= (end - start) * Blocks;

    		reindex();
    	}
    }




    template <typename T>
    void insert(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	this->_insert(idx, 1, [&](Int idx){
    		return values;
    	});
    }

    template <typename Adaptor>
    void insert(Int pos, Int processed, Adaptor&& adaptor) {
    	_insert(pos, processed, std::forward<Adaptor>(adaptor));
    }


    template <typename Adaptor>
    void _insert(Int idx, Int inserted, Adaptor&& adaptor)
    {
    	Int size 	  = this->size();
    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <=, size);

    	Codec codec;

    	auto metadata = this->metadata();
    	size_t data_size = metadata->data_size(0);

    	TreeLayout layout = compute_tree_layout(data_size);

    	auto values			 = this->values();
    	Int global_idx  	 = idx * Blocks;
    	size_t insertion_pos = this->locate(layout, values, 0, global_idx, data_size).idx;

    	size_t total_length = 0;

    	for (Int c = 0; c < inserted; c++)
    	{
    		for (Int block = 0; block < Blocks; block++)
    		{
    			auto value = adaptor(block, c);
    			auto len = codec.length(value);

    			total_length += len;
    		}
    	}

    	resize_segments(data_size + total_length);

    	values = this->values();

    	codec.move(values, insertion_pos, insertion_pos + total_length, data_size - insertion_pos);

    	for (Int c = 0; c < inserted; c++)
    	{
    		for (Int block = 0; block < Blocks; block++)
    		{
    			auto value = adaptor(block, c);
    			Int len = codec.encode(values, value, insertion_pos);
    			insertion_pos += len;
    		}
    	}

    	metadata->data_size(0) += total_length;

    	metadata->size() += (inserted * Blocks);

    	reindex();
    }


    SizesT positions(Int idx) const
    {
    	Int size = this->size();

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <=, size);

    	Int data_size		= this->data_size();
    	auto values			= this->values();
    	TreeLayout layout 	= compute_tree_layout(data_size);

    	SizesT pos;
    	for (Int block = 0; block < Blocks; block++)
    	{
    		pos[block] = this->locate(layout, values, 0, idx * Blocks + block).idx;
    	}

    	return pos;
    }

    SizesT capacities() const
    {
    	return SizesT(0);
    }


    Int insert_buffer(Int at, const InputBuffer* buffer, Int start, Int size)
    {
    	if (size > 0) {
    		Codec codec;

    		auto meta = this->metadata();

    		size_t data_size = meta->data_size(0);

    		Int buffer_start = buffer->locate(0, start);
    		Int buffer_end = buffer->locate(Blocks - 1, start + size);

    		Int total_length = buffer_end - buffer_start;

    		resize_segments(data_size + total_length);

    		auto values 		= this->values();
    		auto buffer_values 	= buffer->values(0);

    		size_t insertion_pos = locate(0, at);
    		codec.move(values, insertion_pos, insertion_pos + total_length, data_size - insertion_pos);

    		codec.copy(buffer_values, buffer_start, values, insertion_pos, total_length);

    		meta->data_size(0) += total_length;

    		meta->size() += (size * Blocks);

    		reindex();
    	}


    	return at + size;
    }




    template <typename T>
    void update(Int idx, const core::StaticVector<T, Blocks>& values)
    {
        setValues(idx, values);
    }



    template <Int Offset, Int Size, typename T1, typename T2, template <typename, Int> class AccumItem>
    void _insert(Int idx, const core::StaticVector<T1, Blocks>& values, AccumItem<T2, Size>& accum)
    {
    	insert(idx, values);

    	sum<Offset>(idx, accum);
    }

    template <Int Offset, Int Size, typename T1, typename T2, template <typename, Int> class AccumItem>
    void _update(Int idx, const core::StaticVector<T1, Blocks>& values, AccumItem<T2, Size>& accum)
    {
    	sub<Offset>(idx, accum);

    	update(idx, values);

    	sum<Offset>(idx, accum);
    }


    template <Int Offset, Int Size, typename T1, typename T2, typename I, template <typename, Int> class AccumItem>
    void _update(Int idx, const std::pair<T1, I>& values, AccumItem<T2, Size>& accum)
    {
    	sub<Offset>(idx, accum);

    	this->setValue(values.first, idx, values.second);

    	sum<Offset>(idx, accum);
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void _remove(Int idx, AccumItem<T, Size>& accum)
    {
    	sub<Offset>(idx, accum);
    	remove(idx, idx + 1);
    }

    template <typename UpdateFn>
    void update_values(Int start, Int end, UpdateFn&& update_fn)
    {
    	auto values			= this->values();
		Int data_size 		= this->data_size();
		TreeLayout layout 	= compute_tree_layout(data_size);
		Int size			= this->size();

    	Codec codec;

    	Int starts[Blocks];

    	for (Int block = 0; block < Blocks; block++)
    	{
    		starts[block] = this->locate(layout, values, 0, block * size + start);
    	}

    	Int shift = 0;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int total_delta = 0;

    		for (Int window_start = start; window_start < end; window_start += 32)
    		{
    			Int window_end = (window_start + 32) < end ? window_start + 32 : end;

    			Int old_length = 0;
    			Int new_length = 0;

    			size_t data_start_tmp = starts[block];

    			Value buffer[32];

    			for (Int c = window_start; c < window_end; c++)
    			{
    				Value old_value;
    				auto len = codec.decode(values, old_value, data_start_tmp);

    				auto new_value = update_fn(block, c, old_value);

    				buffer[c - window_start] = new_value;

    				old_length += len;
    				new_length += codec.length(new_value);

    				data_start_tmp += len;
    			}

    			size_t data_start = starts[block] + shift;

    			if (new_length > old_length)
    			{
    				auto delta = new_length - old_length;

    				insert_space(data_start, delta);

        			values = this->values(block);
        			total_delta += delta;
    			}
    			else if (new_length < old_length)
    			{
    				auto delta = old_length - new_length;

    				remove_space(data_start, delta);

        			values = this->values(block);
        			total_delta -= delta;
    			}

    			for (Int c = window_start; c < window_end; c++)
    			{
    				data_start += codec.encode(values, buffer[c], data_start);
    			}
    		}

    		shift += total_delta;
    	}

    	reindex();
    }


    template <typename UpdateFn>
    void update_values(Int start, UpdateFn&& update_fn)
    {
    	Int size 	   = this->size();

    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(start, <, size);

    	Int global_idx = start * Blocks;

    	Codec codec;

    	Int data_size       = this->data_size();
    	auto values			= this->values();
    	TreeLayout layout 	= compute_tree_layout(data_size);

    	size_t insertion_pos = this->locate(layout, values, 0, global_idx, data_size).idx;

    	size_t insertion_pos0 = insertion_pos;

    	Value old_values[Blocks];
    	Value new_values[Blocks];

    	size_t new_length = 0;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		auto len = codec.decode(values, old_values[block], insertion_pos0);
    		new_values[block] = update_fn(block, old_values[block]);
    		insertion_pos0 += len;

    		new_length += codec.length(new_values[block]);
    	}

    	size_t old_length = insertion_pos0 - insertion_pos;

    	if (new_length > old_length)
    	{
    		insert_space(insertion_pos, new_length - old_length);
    		values = this->values();
    	}
    	else if (old_length > new_length)
    	{
    		remove_space(insertion_pos, old_length - new_length);
    		values = this->values();
    	}

    	for (Int block = 0; block < Blocks; block++)
    	{
    		auto len = codec.encode(values, new_values[block], insertion_pos);
    		insertion_pos += len;
    	}

    	reindex();
    }


    template <typename UpdateFn>
    void update_value(Int block, Int start, UpdateFn&& update_fn)
    {
    	Int size 	   = this->size();

    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(start, <, size);

    	Int global_idx = start * Blocks + block;

    	Codec codec;

    	Int data_size       = this->data_size();
    	auto values			= this->values();
    	TreeLayout layout 	= compute_tree_layout(data_size);

    	size_t insertion_pos = this->locate(layout, values, 0, global_idx).idx;

    	Value value;
    	size_t old_length = codec.decode(values, value, insertion_pos);
    	auto new_value    = update_fn(block, value);

    	if (new_value != value)
    	{
    		size_t new_length = codec.length(new_value);

    		if (new_length > old_length)
    		{
    			insert_space(insertion_pos, new_length - old_length);
    			values = this->values();
    		}
    		else if (old_length > new_length)
    		{
    			remove_space(insertion_pos, old_length - new_length);
    			values = this->values();
    		}

    		codec.encode(values, new_value, insertion_pos);

    		reindex();
    	}
    }




    template <typename T>
    void setValues(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	update_values(idx, [&](Int block, auto old_value){return values[block];});
    }

    template <typename T>
    void addValues(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	update_values(idx, [&](Int block, auto old_value){return values[block] + old_value;});
    }

    template <typename T>
    void subValues(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	update_values(idx, [&](Int block, auto old_value){return values[block] + old_value;});
    }


    void addValue(Int block, Int idx, Value value)
    {
    	update_value(block, idx, [&](Int block, auto old_value){return value + old_value;});
    }

    template <typename T, Int Indexes>
    void addValues(Int idx, Int from, Int size, const core::StaticVector<T, Indexes>& values)
    {
    	update_values(idx, [&](Int block, auto old_value){return values[block + from] + old_value;});
    }

    void check() const {}

    void clear()
    {
        if (Base::has_allocator())
        {
            auto alloc = this->allocator();
            Int empty_size = MyType::empty_size();
            alloc->resizeBlock(this, empty_size);
        }

        init();
    }

    void clear(Int start, Int end)
    {
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::generateDataEvents(handler);

    	handler->startStruct();
    	handler->startGroup("VLD_TREE");

    	auto meta = this->metadata();

    	handler->value("SIZE",      &meta->size());
    	handler->value("DATA_SIZE", &meta->data_size(0), TreeBlocks);

    	handler->startGroup("INDEXES", TreeBlocks);

    	for (Int block = 0; block < TreeBlocks; block++)
    	{
    		Int index_size = this->index_size(Base::data_size(block));

    		handler->startGroup("BLOCK_INDEX", block);

    		auto size_indexes  = this->size_index(block);

    		for (Int c = 0; c < index_size; c++)
    		{
    			BigInt indexes[] = {
    				size_indexes[c]
    			};

    			handler->value("INDEX", indexes, 1);
    		}

    		handler->endGroup();
    	}

    	handler->endGroup();


    	handler->startGroup("DATA", meta->size());

    	Int size = this->size();

    	Codec codec;

    	size_t positions[Blocks];
    	for (Int block = 0; block < Blocks; block++) {
    		positions[block] = this->locate(0, block * size);
    	}

    	auto values = this->values();

    	for (Int idx = 0; idx < size; idx++)
    	{
    		Value values_data[Blocks];
    		for (Int block = 0; block < Blocks; block++)
    		{
    			auto len = codec.decode(values, values_data[block], positions[block]);
    			positions[block] += len;
    		}

    		handler->value("TREE_ITEM", values_data, Blocks);
    	}

    	handler->endGroup();

    	handler->endGroup();

    	handler->endStruct();
    }

    void serialize(SerializationData& buf) const
    {
    	Base::serialize(buf);

    	auto meta = this->metadata();

    	FieldFactory<Int>::serialize(buf, meta->size());

    	FieldFactory<Int>::serialize(buf, meta->data_size(0), TreeBlocks);

        for (Int block = 0; block < TreeBlocks; block++)
        {
        	Base::template serializeSegment<Int>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
        	Base::template serializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);
        	FieldFactory<ValueData>::serialize(buf, Base::values(block), Base::data_size(block));
        }
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto meta = this->metadata();

    	FieldFactory<Int>::deserialize(buf, meta->size());

    	FieldFactory<Int>::deserialize(buf, meta->data_size(0), TreeBlocks);

    	for (Int block = 0; block < TreeBlocks; block++)
        {
        	Base::template deserializeSegment<Int>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
        	Base::template deserializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);
        	FieldFactory<ValueData>::deserialize(buf, Base::values(block), Base::data_size(block));
        }
    }


    template <typename Fn>
    Int read(Int block, Int start, Int end, Fn&& fn) const {
    	return scan(block, start, end, std::forward<Fn>(fn));
    }



    template <typename ConsumerFn>
    Int scan(Int block, Int start, Int end, ConsumerFn&& fn) const
    {
    	Int size = this->size();
    	Int global_idx = block * size + start;

    	size_t pos = this->locate(0, global_idx);
    	size_t data_size = this->data_size();

    	Codec codec;

    	auto values = this->values();

    	Int c;
    	for (c = start; c < end && pos < data_size; c++)
    	{
    		Value value;
    		auto len = codec.decode(values, value, pos);
    		fn(value);
    		pos += len;
    	}

    	return c;
    }


    template <typename T>
    void read(Int block, Int start, Int end, T* values) const
    {
    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(start, <=, end);
    	MEMORIA_ASSERT(end, <=, size());

    	scan(block, start, end, [&](Int c, auto value){
    		values[c - start] = value;
    	});
    }


    void dump(std::ostream& out = cout) const
    {
    	Int size = this->size();

    	auto data_size  = this->data_size();

    	out<<"size_         = "<<size<<std::endl;
    	out<<"block_size_   = "<<this->block_size()<<std::endl;
    	out<<"data_size_    = "<<data_size<<std::endl;

    	auto index_size = this->index_size(data_size);

    	out<<"index_size_   = "<<index_size<<std::endl;

    	TreeLayout layout = this->compute_tree_layout(data_size);

    	if (layout.levels_max >= 0)
    	{
    		out<<"TreeLayout: "<<endl;

    		out<<"Level sizes: ";
    		for (Int c = 0; c <= layout.levels_max; c++) {
    			out<<layout.level_sizes[c]<<" ";
    		}
    		out<<endl;

    		out<<"Level starts: ";
    		for (Int c = 0; c <= layout.levels_max; c++) {
    			out<<layout.level_starts[c]<<" ";
    		}
    		out<<endl;

    		auto size_indexes = this->size_index(0);

    		out<<"Index:"<<endl;
    		for (Int c = 0; c < index_size; c++)
    		{
    			out<<c<<": "<<size_indexes[c]<<std::endl;
    		}
    	}

    	out<<endl;

    	out<<"Offsets: ";
    	for (Int c = 0; c <= this->divUpV(data_size); c++) {
    		out<<this->offset(0, c)<<" ";
    	}
    	out<<endl;

    	out<<"Values: "<<endl;

    	auto values = this->values();

    	Codec codec;

    	size_t pos = 0;

    	for (Int c = 0; c < size; c++)
    	{
    		out<<"c: "<<c<<" ";
    		for (Int block = 0; block < Blocks; block++)
    		{
    			Value value;
    			auto len = codec.decode(values, value, pos);
    			out<<value<<" ";
    			pos += len;
    		}
    		out<<endl;
    	}
    }
};




template <typename Types>
struct PkdStructSizeType<PkdVDArray<Types>> {
	static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


template <typename Types>
struct StructSizeProvider<PkdVDArray<Types>> {
    static const Int Value = 0;
};

template <typename Types>
struct IndexesSize<PkdVDArray<Types>> {
	static const Int Value = 0;
};


}


#endif
