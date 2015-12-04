
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_INPUT_BUFFER_ROW_ORDER_HPP_
#define MEMORIA_CORE_PACKED_VLE_INPUT_BUFFER_ROW_ORDER_HPP_

#include <memoria/core/packed/tree/vle/packed_vle_quick_tree_base.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_tools.hpp>
#include <memoria/core/packed/buffer/packed_vle_input_buffer_co.hpp>

#include <memoria/core/packed/array/packed_vle_array_base.hpp>

namespace memoria {




template <typename Types>
class PkdVLERowOrderInputBuffer: public PkdVLEArrayBase<
	1,
	typename Types::Value,
	Types::template Codec,
	Types::BranchingFactor,
	Types::ValuesPerBranch,
	PkdVLEInputBufferMetadata<1>
> {

	using Base = PkdVLEArrayBase<
			1,
			typename Types::Value,
			Types::template Codec,
			Types::BranchingFactor,
			Types::ValuesPerBranch,
			PkdVLEInputBufferMetadata<1>
	>;

	using MyType = PkdVLERowOrderInputBuffer<Types>;

public:
    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;
    using Base::reindex_block;
    using Base::offsets_segment_size;
    using Base::locate;
    using Base::block_size;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;
    using Base::BITS_PER_DATA_VALUE;
    using Base::ValuesPerBranch;


    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;



    using typename Base::Codec;

    static constexpr UInt VERSION = 1;
    static constexpr Int TreeBlocks = 1;
    static constexpr Int Blocks = Types::Blocks;

    static constexpr Int SafetyMargin = (128 / Codec::ElementSize) * Blocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<UInt, Blocks>
    >;

    using Value		 = typename Types::Value;

    using Values = core::StaticVector<Value, Blocks>;

    using SizesT = core::StaticVector<Int, Blocks>;

    void init(const SizesT& sizes)
    {
    	Base::init(empty_size(), TreeBlocks * SegmentsPerBlock + BlocksStart);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);

    	meta->size() = 0;

    	Int capacity        = sizes.sum();
    	Int offsets_size 	= offsets_segment_size(capacity);
    	Int index_size		= this->index_size(capacity);
    	Int values_segment_length = this->value_segment_size(capacity);

    	meta->max_data_size(0) = capacity;

    	Int block = 0;
    	this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
    	this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
    	this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);

    }



    void init()
    {
    	Base::init(empty_size(), TreeBlocks * SegmentsPerBlock + BlocksStart);

    	this->template allocate<Metadata>(METADATA);

    	Int offsets_size = offsets_segment_size(0);

    	Int block = 0;
    	this->template allocateArrayBySize<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0);
    	this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
    	this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + VALUES + BlocksStart, 0);
    }

    static Int block_size(Int capacity)
    {
    	return Base::block_size_equi(TreeBlocks, capacity * Blocks);
    }


    static Int block_size(const SizesT& capacities)
    {
    	Int capacity 			= capacities.sum();
    	Int metadata_length 	= Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
    	Int data_sizes_length 	= Base::roundUpBytesToAlignmentBlocks(TreeBlocks * sizeof(Int));

    	Int index_size      = MyType::index_size(capacity);
    	Int sizes_length	= Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(Int));

    	Int values_length   = Base::roundUpBitsToAlignmentBlocks(capacity * BITS_PER_DATA_VALUE);

    	Int offsets_length 	= offsets_segment_size(capacity);

    	Int segments_length = values_length + offsets_length + sizes_length;

    	return PackedAllocator::block_size (
    			metadata_length +
				data_sizes_length +
				segments_length,
				TreeBlocks * SegmentsPerBlock + BlocksStart
    	);
    }


    Int block_size() const
    {
    	return Base::block_size();
    }



    Int locate(Int block, Int idx) const
    {
    	auto meta = this->metadata();

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <=, meta->size());

    	auto values = this->values(0);
    	auto data_size = meta->data_size(0);

    	TreeLayout layout = Base::compute_tree_layout(meta->max_data_size(0));

    	auto pos = locate(layout, values, 0, idx * Blocks, data_size).idx;

    	if (idx < meta->size() / Blocks)
    	{
    		Codec codec;

    		for (Int b = 0; b < block; b++)
    		{
    			pos += codec.length(values, pos, data_size);
    		}
    	}

    	return pos;
    }




    static Int empty_size()
    {
    	return block_size(0);
    }

    void reindex()
    {
    	auto metadata = this->metadata();
    	for (Int block = 0; block < TreeBlocks; block++)
    	{
    		auto data_size = metadata->data_size(block);
        	TreeLayout layout = this->compute_tree_layout(metadata->max_data_size(block));
        	Base::reindex_block(block, layout, data_size);
    	}
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

    void reset() {
    	auto meta = this->metadata();
    	meta->data_size().clear();
    }

    Value value(Int block, Int idx) const
    {
    	auto start_pos = locate(block, idx);
    	auto values = this->values(0);

		Codec codec;
		Value value;

		codec.decode(values, value, start_pos);

		return value;
    }

    Values get_values(Int idx) const
    {
    	auto start_pos = locate(0, idx);
    	auto values = this->values(0);

		Codec codec;
		Values data;

    	for (Int b = 0; b < Blocks; b++)
    	{
    		start_pos += codec.decode(values, data[b], start_pos);
    	}

		return data;
    }

    Value get_values(Int idx, Int index) const
    {
    	return this->value(index, idx);
    }

    Value getValue(Int index, Int idx) const
    {
    	return this->value(index, idx);
    }

    SizesT positions(Int idx) const
    {
    	auto start_pos = locate(0, idx);
    	auto values	   = this->values(0);

    	Codec codec;
    	SizesT pos;
    	for (Int block = 0; block < Blocks; block++)
    	{
    		pos[block] = start_pos;

    		start_pos += codec.length(values, start_pos, -1);
    	}

    	return pos;
    }

    core::StaticVector<Int, 1> capacities() const
    {
    	return core::StaticVector<Int, 1>(this->metadata()->capacity()[0]);
    }




    // ========================================= Insert/Remove/Resize ============================================== //

protected:
    void resize(Int block, Int data_size, Int start, Int length)
    {
    	Int new_data_size = data_size + length;

    	Int data_segment_size 	 = PackedAllocator::roundUpBitsToAlignmentBlocks(new_data_size * Codec::ElementSize);
    	Int index_size 	 	   	 = Base::index_size(new_data_size);
    	Int offsets_segment_size = Base::offsets_segment_size(new_data_size);

    	this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, data_segment_size);
    	this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_segment_size);
    	this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
    }


    void insert_space(Int block, Int start, Int length)
    {
    	auto metadata = this->metadata();
    	Int data_size = metadata->data_size(block);
    	auto values   = this->values(block);

    	Codec codec;
    	codec.move(values, start, start + length, data_size - start);

    	metadata->data_size(block) += length;

    	MEMORIA_ASSERT(metadata->data_size(block), <=, metadata->max_data_size(block));
    }


    void remove_space(Int block, Int start, Int length)
    {
    	auto metadata = this->metadata();

    	Int data_size = metadata->data_size(block);
    	auto values   = this->values(block);

    	Codec codec;
    	Int end = start + length;
    	codec.move(values, end, start, data_size - end);

    	MEMORIA_ASSERT(data_size + (end - start), <=, metadata->max_data_size(block));

    	metadata->data_size(block) -= (end - start);
    }

public:


    void remove_space(Int start, Int end)
    {
    	remove(start, end);
    }

    void removeSpace(Int start, Int end) {
    	remove(start, end);
    }

    void remove(Int start, Int end)
    {
    	auto metadata = this->metadata();

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int data_size		= metadata->data_size(block);
    		auto values			= this->values(block);
    		TreeLayout layout 	= compute_tree_layout(metadata->max_data_size(block));

    		Int start_pos = this->locate(layout, values, block, start, data_size).idx;
    		Int end_pos   = this->locate(layout, values, block, end, data_size).idx;

    		this->remove_space(block, start_pos, end_pos - start_pos);
    	}

    	this->size() -= end - start;

    	reindex();
    }




    template <typename T>
    void insert(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	this->_insert(idx, 1, [&](Int block, Int idx) {
    		return values[block];
    	});
    }




    template <typename Adaptor>
    void insert(Int pos, Int processed, Adaptor&& adaptor)
    {
    	Int size = this->size();

    	MEMORIA_ASSERT(pos, >=, 0);
    	MEMORIA_ASSERT(pos, <=, size);
    	MEMORIA_ASSERT(processed, >=, 0);

    	Codec codec;

    	SizesT total_lengths;

    	for (SizeT c = 0; c < processed; c++)
    	{
    		for (Int block = 0; block < Blocks; block++)
    		{
    			auto value = adaptor(block, c);
    			auto len = codec.length(value);
    			total_lengths[block] += len;
    		}
    	}

    	auto meta = this->metadata();


    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int data_size		= meta->data_size(block);
    		auto values			= this->values(block);
    		TreeLayout layout 	= compute_tree_layout(meta->max_data_size(block));

    		auto lr = this->locate(layout, values, block, pos, data_size);

    		size_t insertion_pos = lr.idx;

    		this->insert_space(block, insertion_pos, total_lengths[block]);

    		values = this->values(block);

    		for (Int c = 0; c < processed; c++)
    		{
    			auto value = adaptor(block, c);
    			auto len = codec.encode(values, value, insertion_pos);
    			insertion_pos += len;
    		}
    	}

    	this->size() += processed;

    	reindex();
    }




    template <typename Adaptor>
    SizesT populate(const SizesT& at, Int size, Adaptor&& adaptor)
    {
    	Codec codec;

    	SizesT total_lengths;

    	for (Int c = 0; c < size; c++)
    	{
    		for (Int block = 0; block < Blocks; block++)
    		{
    			total_lengths[block] += codec.length(adaptor(block, c));
    		}
    	}

    	for (Int block = 0; block < Blocks; block++)
    	{
    		size_t insertion_pos = at[block];

    		auto values = this->values(block);

    		for (Int c = 0; c < size; c++)
    		{
    			auto value = adaptor(block, c);
    			auto len = codec.encode(values, value, insertion_pos);
    			insertion_pos += len;
    		}

    		this->data_size(block) += total_lengths[block];
    	}

    	this->size() += size;

    	return at + total_lengths;
    }


    template <typename Adaptor>
    void append(SizesT& at, Int size, Adaptor&& adaptor)
    {
    	Codec codec;

    	auto metadata = this->metadata();

    	auto values = this->values(0);

    	size_t pos = at[0];

    	for (Int c = 0; c < size; c++)
    	{
    		auto value = adaptor(c);

    		for (Int block = 0; block < Blocks; block++)
    		{
    			pos += codec.encode(values, value[block], pos);
    		}
    	}

    	metadata->data_size(0) += pos - at[0];
    	at[0] = pos;

    	this->size() += size;
    }


    void check_indexless(Int block, Int data_size) const
    {
    	Int offsets_size = this->element_size(block * SegmentsPerBlock + Base::OFFSETS + BlocksStart);
    	MEMORIA_ASSERT(this->element_size(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart), ==, 0);

    	if (data_size > 0)
    	{
    		MEMORIA_ASSERT(offsets_size, ==, sizeof(OffsetsType));
    		MEMORIA_ASSERT(this->offset(block, 0), ==, 0);
    	}
    	else {
    		MEMORIA_ASSERT(offsets_size, ==, 0);
    	}

    	MEMORIA_ASSERT(data_size, <=, ValuesPerBranch);
    }


    void check() const
    {
    	auto metadata = this->metadata();

    	for (Int block = 0; block < TreeBlocks; block++)
    	{
    		Int data_size 	  = metadata->data_size(block);
    		Int max_data_size = metadata->max_data_size(block);

    		MEMORIA_ASSERT(data_size, <=, max_data_size);

    		TreeLayout layout = this->compute_tree_layout(max_data_size);

    		if (layout.levels_max >= 0)
    		{
    			Base::check_block(block, layout, data_size);
    		}
    		else {
    			check_indexless(block, max_data_size);
    		}
    	}
    }

    template <typename ConsumerFn>
    SizesT scan(Int start, Int end, ConsumerFn&& fn) const
    {
    	auto metadata = this->metadata();

    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(end, >=, start);
    	MEMORIA_ASSERT(end, <=, metadata->size());

    	Codec codec;

    	auto values = this->values(0);
    	size_t position = this->locate(0, start);

    	for (Int c = start; c < end; c++)
    	{
    		Values values_data;

    		for (Int block = 0; block < Blocks; block++)
    		{
    			position += codec.decode(values, values_data[block], position);
    		}

    		fn(values_data);
    	}

    	return SizesT(position);
    }

    void dump(std::ostream& out = cout) const
    {
    	auto metadata = this->metadata();

    	Int size = metadata->size();

    	auto data_size  	= metadata->data_size(0);
    	auto max_data_size  = metadata->max_data_size(0);

    	out<<"size_         = "<<size<<std::endl;
    	out<<"block_size_   = "<<this->block_size()<<std::endl;
    	out<<"data_size_    = "<<data_size<<std::endl;
    	out<<"max_data_size_= "<<max_data_size<<std::endl;

    	auto index_size = this->index_size(max_data_size);

    	out<<"index_size_   = "<<index_size<<std::endl;

    	TreeLayout layout = this->compute_tree_layout(max_data_size);

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

    	auto values = this->values(0);

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




}


#endif
