
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_INPUT_BUFFER_COLUMN_ORDER_HPP_
#define MEMORIA_CORE_PACKED_VLE_INPUT_BUFFER_COLUMN_ORDER_HPP_

#include <memoria/core/packed/tree/vle/packed_vle_quick_tree_base.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_tools.hpp>

#include <memoria/core/packed/array/packed_vle_array_base.hpp>

namespace memoria {


template <Int Blocks>
class PkdVLEInputBufferMetadata {
	using SizesT = core::StaticVector<Int, Blocks>;

	Int size_;

	SizesT data_size_;
	SizesT max_data_size_;

public:
	PkdVLEInputBufferMetadata() = default;

	Int& size() {return size_;}
	const Int& size() const {return size_;}

	Int& data_size(Int block) {return data_size_[block];}
	const Int& data_size(Int block) const {return data_size_[block];}

	Int& max_data_size(Int block) {return max_data_size_[block];}
	const Int& max_data_size(Int block) const {return max_data_size_[block];}

	SizesT capacity() const {
		return max_data_size_ - data_size_;
	}

	template <Int, typename, template <typename> class Codec, Int, Int, typename> friend class PkdVLEArrayBase;
};


template <typename Types>
class PkdVLEColumnOrderInputBuffer: public PkdVLEArrayBase<
	Types::Blocks,
	typename Types::Value,
	Types::template Codec,
	Types::BranchingFactor,
	Types::ValuesPerBranch,
	PkdVLEInputBufferMetadata<Types::Blocks>
> {

	using Base = PkdVLEArrayBase<
			Types::Blocks,
			typename Types::Value,
			Types::template Codec,
			Types::BranchingFactor,
			Types::ValuesPerBranch,
			PkdVLEInputBufferMetadata<Types::Blocks>
	>;

	using MyType = PkdVLEColumnOrderInputBuffer<Types>;

public:
    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;
    using Base::reindex_block;
    using Base::offsets_segment_size;
    using Base::locate;

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
    static constexpr Int Blocks = Types::Blocks;

    static constexpr Int SafetyMargin = 128 / Codec::ElementSize;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<UInt, Blocks>
    >;

    using IndexValue = typename Types::IndexValue;
    using Value		 = typename Types::Value;

    using Values = core::StaticVector<IndexValue, Blocks>;

    using InputBuffer 	= MyType;
    using InputType 	= Values;

    using SizesT = core::StaticVector<Int, Blocks>;

    void init(const SizesT& sizes)
    {
    	Base::init(empty_size(), Blocks * SegmentsPerBlock + BlocksStart);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);

    	meta->size() = 0;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int capacity        = sizes[block];
    		Int offsets_size 	= offsets_segment_size(capacity);
    		Int index_size		= this->index_size(capacity);
    		Int values_segment_length = this->value_segment_size(capacity);

    		meta->max_data_size(block) = capacity;

    		this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
    		this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
    		this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
    	}
    }



    void init()
    {
    	Base::init(empty_size(), Blocks * SegmentsPerBlock + BlocksStart);

    	this->template allocate<Metadata>(METADATA);

    	Int offsets_size = offsets_segment_size(0);

    	for (Int block = 0; block < Blocks; block++)
    	{
    		this->template allocateArrayBySize<Int>(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, 0);
    		this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
    		this->template allocateArrayBySize<Byte>(block * SegmentsPerBlock + VALUES + BlocksStart, 0);
    	}
    }

    static Int block_size(Int capacity)
    {
    	return Base::block_size_equi(Blocks, capacity);
    }


    static Int block_size(const SizesT& capacity)
    {
    	Int metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
    	Int data_sizes_length = Base::roundUpBytesToAlignmentBlocks(Blocks * sizeof(Int));


    	Int segments_length = 0;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int index_size      = MyType::index_size(capacity[block]);
    		Int sizes_length	= Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(Int));

    		Int values_length   = Base::roundUpBitsToAlignmentBlocks(capacity[block] * BITS_PER_DATA_VALUE);

    		Int offsets_length 	= offsets_segment_size(capacity[block]);

    		segments_length += values_length + offsets_length + sizes_length;
    	}

    	return PackedAllocator::block_size(
    			metadata_length +
				data_sizes_length +
				segments_length,
				Blocks * SegmentsPerBlock + BlocksStart
    	);
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
        return block_size(this->data_size_v() + other->data_size_v());
    }


    SizesT data_size_v() const
    {
    	SizesT sizes;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		sizes[block] = this->data_size(block);
    	}

    	return sizes;
    }


    static Int elements_for(Int block_size)
    {
        return Base::tree_size(Blocks, block_size);
    }

    static Int expected_block_size(Int items_num)
    {
        return block_size(items_num);
    }

    Value value(Int block, Int idx) const
    {
    	auto meta = this->metadata();

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <, this->size());

    	Int data_size	  = meta->data_size(block);
    	auto values 	  = this->values(block);
    	TreeLayout layout = this->compute_tree_layout(meta->max_data_size(block));

		Int start_pos  	  = this->locate(layout, values, block, idx, data_size).idx;

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

    void reindex()
    {
    	auto metadata = this->metadata();
    	for (Int block = 0; block < Blocks; block++)
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

    Values get_values(Int idx) const
    {
    	Values v;

    	for (Int i = 0; i < Blocks; i++)
    	{
    		v[i] = this->value(i, idx);
    	}

    	return v;
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
    	auto metadata = this->metadata();

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <=, this->size());

    	SizesT pos;
    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int data_size		= metadata->data_size(block);
    		auto values			= this->values(block);
    		TreeLayout layout 	= compute_tree_layout(metadata->max_data_size(block));

    		pos[block] = this->locate(layout, values, block, idx, data_size).idx;
    	}
    	return pos;
    }

    SizesT capacities() const
    {
    	return this->metadata()->capacity();
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
    	SizesT at0 = at;

    	ValueData* values[Blocks];
    	for (Int block = 0; block < Blocks; block++)
    	{
    		values[block] = this->values(block);
    	}

    	for (Int c = 0; c < size; c++)
    	{
    		auto value = adaptor(c);

    		for (Int block = 0; block < Blocks; block++)
    		{
    			at[block] += codec.encode(values[block], value[block], at[block]);
    		}
    	}

    	for (Int block = 0; block < Blocks; block++)
    	{
    		metadata->data_size(block) += at[block] - at0[block];
    	}

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

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int data_size 	  = metadata->data_size(block);
    		Int max_data_size = metadata->max_data_size(block);
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
    Int scan(Int block, Int start, Int end, ConsumerFn&& fn) const
    {
    	auto meta = this->metadata();

    	auto values = this->values(block);
    	size_t data_size = meta->data_size(block);

    	TreeLayout layout = this->compute_tree_layout(meta->max_data_size(block));
    	size_t pos = this->locate(layout, values, block, start, data_size).idx;


    	Codec codec;

    	Int c;
    	for (c = start; c < end && pos < data_size; c++)
    	{
    		Value value;
    		auto len = codec.decode(values, value, pos);
    		fn(c, value);
    		pos += len;
    	}

    	return c;
    }


    template <typename ConsumerFn>
    SizesT scan(Int start, Int end, ConsumerFn&& fn) const
    {
    	auto metadata = this->metadata();

    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(end, >=, start);
    	MEMORIA_ASSERT(end, <=, metadata->size());

    	Codec codec;

    	SizesT position;
    	const ValueData* values[Blocks];

    	for (Int block = 0; block < Blocks; block++)
    	{
    		values[block] = this->values(block);
    		auto data_size = metadata->data_size(block);

    		TreeLayout layout = this->compute_tree_layout(metadata->max_data_size(block));
    		position[block] = this->locate(layout, values[block], block, start, data_size).idx;
    	}

    	for (Int c = start; c < end; c++)
    	{
    		Values values_data;

    		for (Int block = 0; block < Blocks; block++)
    		{
    			position[block] += codec.decode(values[block], values_data[block], position[block]);
    		}

    		fn(values_data);
    	}

    	return position;
    }

    void dump(std::ostream& out = cout) const
    {
    	auto meta = this->metadata();
    	auto size = meta->size();

    	out<<"size_         = "<<size<<std::endl;
    	out<<"block_size_   = "<<this->block_size()<<std::endl;

    	for (Int block = 0; block < Blocks; block++) {
    		out<<"data_size_["<<block<<"] = "<<this->data_size(block)<<std::endl;
    	}

    	for (Int block = 0; block < Blocks; block++)
    	{
    		out<<"++++++++++++++++++ Block: "<<block<<" ++++++++++++++++++"<<endl;

    		auto data_size  = this->data_size(block);
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

    			auto size_indexes = this->size_index(block);

    			out<<"Index:"<<endl;
    			for (Int c = 0; c < index_size; c++)
    			{
    				out<<c<<": "<<size_indexes[c]<<std::endl;
    			}
    		}

    		out<<endl;

    		out<<"Offsets: ";
    		for (Int c = 0; c <= this->divUpV(data_size); c++) {
    			out<<this->offset(block, c)<<" ";
    		}
    		out<<endl;
    	}

    	out<<"Values: "<<endl;

    	const ValueData* values[Blocks];
    	size_t block_pos[Blocks];

    	for (Int block = 0; block < Blocks; block++) {
    		values[block] = this->values(block);
    		block_pos[block] = 0;
    	}

    	Codec codec;
    	for (Int c = 0; c < size; c++)
    	{
    		out<<c<<": "<<c<<" ";
    		for (Int block = 0; block < Blocks; block++)
    		{
    			Value value;
    			auto len = codec.decode(values[block], value, block_pos[block]);

    			out<<"  ("<<block_pos[block]<<") "<<value;
    			block_pos[block] += len;
    		}
    		out<<endl;
    	}
    }
};




}


#endif
