
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_QUICK_TREE_HPP_
#define MEMORIA_CORE_PACKED_VLE_QUICK_TREE_HPP_

#include <memoria/core/packed/tree/packed_vle_quick_tree_base.hpp>


namespace memoria {

template <typename Codec>
struct PkdVQTreeShapeProvider {
	static constexpr Int BitsPerElement = Codec::ElementSize;
	static constexpr Int BlockSize = 128;// bytes

	static constexpr Int BranchingFactor = PackedTreeBranchingFactor;
	static constexpr Int ValuesPerBranch = BlockSize * 8 / BitsPerElement;
};


template <
	typename IndexValueT,
	Int kBlocks,
	template <typename> class CodecT,
	typename ValueT = BigInt,
	Int kBranchingFactor = PkdVQTreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
	Int kValuesPerBranch = PkdVQTreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
class PkdVQTree: public PkdVQTreeBase<IndexValueT, ValueT, CodecT, kBranchingFactor, kValuesPerBranch> {

	using Base 		= PkdVQTreeBase<IndexValueT, ValueT, CodecT, kBranchingFactor, kValuesPerBranch>;
	using MyType 	= PkdVQTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>;

public:
    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;
    using Base::gsum;
    using Base::find;
    using Base::walk_fw;
    using Base::walk_bw;
    using Base::reindex_block;
    using Base::offsets_segment_size;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::VALUE_INDEX;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;
    using Base::BITS_PER_DATA_VALUE;


    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;
    using typename Base::FindGEWalker;
    using typename Base::FindGTWalker;


    using typename Base::Codec;

    static constexpr UInt VERSION = 1;
    static constexpr Int Blocks = kBlocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<UInt, Blocks>
    >;

    using Values = core::StaticVector<IndexValueT, Blocks>;

    using InputBuffer 	= MyType;
    using InputType 	= Values;

    using SizesT = core::StaticVector<Int, Blocks>;

    static Int estimate_block_size(Int tree_capacity, Int density_hi = 1000, Int density_lo = 333)
    {
        Int max_tree_capacity = (tree_capacity * Blocks * density_hi) / density_lo;
        return block_size(max_tree_capacity);
    }


    void init_tl(Int data_block_size)
    {
    	Base::init_tl(data_block_size, Blocks);
    }

    void init(const SizesT& sizes)
    {
    	Base::init(empty_size(), Blocks * SegmentsPerBlock + BlocksStart);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);
    	this->template allocateArrayBySize<Int>(DATA_SIZES, Blocks);

    	meta->size()        = 0;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int capacity        = sizes[block];
    		Int offsets_size 	= offsets_segment_size(capacity);
    		Int index_size		= this->index_size(capacity);
    		Int values_segment_length = this->value_segment_size(capacity);

    		this->resizeBlock(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size * sizeof(IndexValueT));
    		this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
    		this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_size);
    		this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, values_segment_length);
    	}
    }



    void init()
    {
    	Base::init(empty_size(), Blocks * SegmentsPerBlock + BlocksStart);

    	Metadata* meta = this->template allocate<Metadata>(METADATA);
    	this->template allocateArrayBySize<Int>(DATA_SIZES, Blocks);

    	meta->size() = 0;
    	Int offsets_size = offsets_segment_size(0);

    	for (Int block = 0; block < Blocks; block++)
    	{
    		this->template allocateArrayBySize<IndexValueT>(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, 0);
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
    		Int index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValueT));
    		Int sizes_length	= Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(Int));

    		Int values_length   = Base::roundUpBitsToAlignmentBlocks(capacity[block] * BITS_PER_DATA_VALUE);

    		Int offsets_length 	= offsets_segment_size(capacity[block]);

    		segments_length += index_length + values_length + offsets_length + sizes_length;
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

    ValueT value(Int block, Int idx) const
    {
    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <, this->size());

    	Int data_size	  = this->data_size(block);
    	auto values 	  = this->values(block);
    	TreeLayout layout = this->compute_tree_layout(data_size);

		Int start_pos  	  = this->locate(layout, values, block, idx).idx;

		MEMORIA_ASSERT(start_pos, <, data_size);

		Codec codec;
		ValueT value;

		codec.decode(values, value, start_pos);

		return value;
    }


    static Int empty_size()
    {
    	return block_size(0);
    }

    void reindex() {
    	Base::reindex(Blocks);
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

    Values sums(Int from, Int to) const
    {
    	Values vals;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		vals[block] = this->sum(block, from, to);
    	}

    	return vals;
    }


    Values sums() const
    {
    	Values vals;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		vals[block] = this->sum(block);
    	}

    	return vals;
    }

    template <typename T>
    void sums(Int from, Int to, core::StaticVector<T, Blocks>& values) const
    {
    	values += this->sums(from, to);
    }

    void sums(Values& values) const
    {
        values += this->sums();
    }


    void sums(Int idx, Values& values) const
    {
        addKeys(idx, values);
    }



    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

    	for (Int block = 0; block < Blocks; block++)
    	{
    		accum[block + Offset] += this->sum(block);
    	}
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int start, Int end, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

    	for (Int block = 0; block < Blocks; block++)
    	{
    		accum[block + Offset] += this->sum(block, start, end);
    	}
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int idx, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

    	for (Int block = 0; block < Blocks; block++)
    	{
    		accum[block + Offset] += this->value(block, idx);
    	}
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sub(Int idx, AccumItem<T, Size>& accum) const
    {
    	static_assert(Offset <= Size - Blocks, "Invalid balanced tree structure");

    	for (Int block = 0; block < Blocks; block++)
    	{
    		accum[block + Offset] -= this->value(block, idx);
    	}
    }


    template <Int Offset, Int From, Int To, typename T, template <typename, Int, Int> class AccumItem>
    void sum(Int start, Int end, AccumItem<T, From, To>& accum) const
    {
    	for (Int block = 0; block < Blocks; block++)
    	{
    		accum[block + Offset] += this->sum(block, start, end);
    	}
    }


    template <typename T>
    void _add(Int block, T& value) const
    {
    	value += this->sum(block);
    }

    template <typename T>
    void _add(Int block, Int end, T& value) const
    {
    	value += this->sum(block, end);
    }

    template <typename T>
    void _add(Int block, Int start, Int end, T& value) const
    {
    	value += this->sum(block, start, end);
    }



    template <typename T>
    void _sub(Int block, T& value) const
    {
    	value -= this->sum(block);
    }

    template <typename T>
    void _sub(Int block, Int end, T& value) const
    {
    	value -= this->sum(block, end);
    }

    template <typename T>
    void _sub(Int block, Int start, Int end, T& value) const
    {
    	value -= this->sum(block, start, end);
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

    ValueT get_values(Int idx, Int index) const
    {
    	return this->value(index, idx);
    }

    ValueT getValue(Int index, Int idx) const
    {
    	return this->value(index, idx);
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
    	this->resizeBlock(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size * sizeof(IndexValueT));
    }


    void insert_space(Int block, Int start, Int length)
    {
    	Int data_size = this->data_size(block);
    	resize(block, data_size, start, length);

    	auto values = this->values(block);

    	Codec codec;
    	codec.move(values, start, start + length, data_size - start);

    	this->data_size(block) += length;
    }

    void dump_values(Int block, std::ostream& out = std::cout)
    {
    	out<<"Dump values"<<std::endl;
    	Codec codec;
    	size_t pos = 0;

    	auto values 	= this->values(block);
    	auto data_size 	= this->data_size(block);

    	for(Int c = 0; pos < data_size; c++)
    	{
    		ValueT value;
    		auto len = codec.decode(values, value, pos);

    		out<<c<<": "<<pos<<" "<<value<<std::endl;

    		pos += len;
    	}

    	out<<std::endl;
    }


    void remove_space(Int block, Int start, Int length)
    {
    	Int data_size = this->data_size(block);
    	auto values = this->values(block);

    	Codec codec;
    	Int end = start + length;
    	codec.move(values, end, start, data_size - end);

    	resize(block, data_size, start, -(end - start));

    	this->data_size(block) -= (end - start);
    }



public:
    void splitTo(MyType* other, Int idx)
    {
    	Codec codec;
    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int start = this->locate(block, idx);
    		Int size  = this->data_size(block) - start;

    		other->insert_space(block, 0, size);
    		codec.copy(this->values(block), start, other->values(block), 0, size);
    	}

    	Int size = this->size();
        other->size() += size - idx;

        other->reindex();

        remove(idx, size);
    }


    void mergeWith(MyType* other)
    {
    	Codec codec;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int data_size = this->data_size(block);
    		Int other_data_size = other->data_size(block);
    		Int start = other_data_size;
    		other->insert_space(block, other_data_size, data_size);

    		codec.copy(this->values(block), 0, other->values(block), start, data_size);
    	}

    	other->size() += this->size();

    	other->reindex();

    	this->clear();
    }



    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
    	Codec codec;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int data_size = this->data_size(block);
    		other->insertSpace(block, 0, data_size);
    		codec.copy(this->values(block), 0, other->values(block), 0, data_size);
    	}

    	other->reindex();
    }


    void remove_space(Int start, Int end)
    {
    	remove(start, end);
    }

    void removeSpace(Int start, Int end) {
    	remove(start, end);
    }

    void remove(Int start, Int end)
    {
    	for (Int block = 0; block < Blocks; block++)
    	{
    		const Int data_size	= this->data_size(block);
    		auto values			= this->values(block);
    		TreeLayout layout 	= compute_tree_layout(data_size);

    		Int start_pos = this->locate(layout, values, block, start).idx;
    		Int end_pos   = this->locate(layout, values, block, end).idx;

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
    void insert(Int pos, Int processed, Adaptor&& adaptor) {
    	_insert(pos, processed, std::forward<Adaptor>(adaptor));
    }


    template <typename Adaptor>
    void _insert(Int pos, Int processed, Adaptor&& adaptor)
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


    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int data_size		= this->data_size(block);
    		auto values			= this->values(block);
    		TreeLayout layout 	= compute_tree_layout(data_size);

    		auto lr = this->locate(layout, values, block, pos);

    		size_t insertion_pos = lr.idx;

    		this->insert_space(block, insertion_pos, total_lengths[block]);

    		values = this->values(block);

    		for (Int c = 0; c < processed; c++)
    		{
    			auto value = adaptor(block, c);
    			Int len = codec.encode(values, value, insertion_pos);
    			insertion_pos += len;
    		}
    	}

    	this->size() += processed;

    	reindex();
    }


    template <typename Adaptor>
    SizesT populate(const SizesT& at, const SizesT& total_lengths, Int size, Adaptor&& adaptor)
    {
    	Codec codec;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		size_t insertion_pos = at[block];

    		auto values = this->values(block);

    		for (Int c = 0; c < size; c++)
    		{
    			auto value = adaptor(block, c);
    			Int len = codec.encode(values, value, insertion_pos);
    			insertion_pos += len;
    		}

    		this->data_size(block) += total_lengths[block];
    	}

    	this->size() += size;

    	return at + total_lengths;
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
    			Int len = codec.encode(values, value, insertion_pos);
    			insertion_pos += len;
    		}

    		this->data_size(block) += total_lengths[block];
    	}

    	this->size() += size;

    	return at + total_lengths;
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
    	Codec codec;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		auto values			= this->values(block);
    		Int data_size 		= this->data_size(block);
    		TreeLayout layout 	= compute_tree_layout(data_size);
    		size_t data_start	= this->locate(layout, values, block, start);

    		for (Int window_start = start; window_start < end; window_start += 32)
    		{
    			Int window_end = (window_start + 32) < end ? window_start + 32 : end;

    			Int old_length = 0;
    			Int new_length = 0;

    			auto values	= this->values(block);

    			size_t data_start_tmp = data_start;

    			ValueT buffer[32];

    			for (Int c = window_start; c < window_end; c++)
    			{
    				ValueT old_value;
    				auto len = codec.decode(values, old_value, data_start_tmp, data_size);

    				auto new_value = update_fn(block, c, old_value);

    				buffer[c - window_start] = new_value;

    				old_length += len;
    				new_length += codec.length(new_value);

    				data_start_tmp += len;
    			}

    			if (new_length > old_length)
    			{
    				auto delta = new_length - old_length;
    				this->insert_space(block, data_start, delta);

        			values = this->values(block);
    			}
    			else if (new_length < old_length)
    			{
    				auto delta = old_length - new_length;
    				this->remove_space(block, data_start, delta);

        			values = this->values(block);
    			}

    			for (Int c = window_start; c < window_end; c++)
    			{
    				data_start += codec.encode(values, buffer[c], data_start);
    			}
    		}

    		reindex_block(block);
    	}
    }


    template <typename UpdateFn>
    void update_values(Int start, UpdateFn&& update_fn)
    {
    	for (Int block = 0; block < Blocks; block++)
    	{
    		update_value(block, start, std::forward<UpdateFn>(update_fn));
    	}
    }


    template <typename UpdateFn>
    void update_value(Int block, Int start, UpdateFn&& update_fn)
    {
    	MEMORIA_ASSERT(start, <=, this->size());
    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(block, >=, 0);
    	MEMORIA_ASSERT(block, <=, Blocks);

    	Codec codec;

    	Int data_size 		= this->data_size(block);

    	auto values			= this->values(block);
    	TreeLayout layout 	= compute_tree_layout(data_size);
    	size_t insertion_pos = this->locate(layout, values, block, start).idx;

    	ValueT value;
    	size_t old_length = codec.decode(values, value, insertion_pos, data_size);
    	auto new_value = update_fn(block, value);

    	if (new_value != value)
    	{
    		size_t new_length = codec.length(new_value);

    		if (new_length > old_length)
    		{
    			this->insert_space(block, insertion_pos, new_length - old_length);
    			values = this->values(block);

    		}
    		else if (old_length > new_length)
    		{
    			this->remove_space(block, insertion_pos, old_length - new_length);
    			values = this->values(block);
    		}

    		codec.encode(values, new_value, insertion_pos);

    		reindex_block(block);
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


    void addValue(Int block, Int idx, ValueT value)
    {
    	update_value(block, idx, [&](Int block, auto old_value){return value + old_value;});
    }

    template <typename T, Int Indexes>
    void addValues(Int idx, Int from, Int size, const core::StaticVector<T, Indexes>& values)
    {
    	for (Int block = 0; block < size; block++)
    	{
    		update_value(block, idx, [&](Int block, auto old_value){return values[block + from] + old_value;});
    	}

    	reindex();
    }




    void check() const {
    	Base::check(Blocks);
    }

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
    	handler->startGroup("VLQ_TREE");

    	auto meta = this->metadata();

    	handler->value("SIZE",      &meta->size());
    	handler->value("DATA_SIZE", this->data_sizes(), Blocks);


    	handler->startGroup("INDEXES", Blocks);

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int index_size = this->index_size(this->data_size(block));

    		handler->startGroup("BLOCK_INDEX", block);

    		auto value_indexes = this->value_index(block);
    		auto size_indexes  = this->size_index(block);

    		for (Int c = 0; c < index_size; c++)
    		{
    			BigInt indexes[] = {
    				value_indexes[c],
					size_indexes[c]
    			};

    			handler->value("INDEX", indexes, 2);
    		}

    		handler->endGroup();
    	}

    	handler->endGroup();


    	handler->startGroup("DATA", meta->size());

    	const ValueData* values[Blocks];
    	for (Int b = 0; b < Blocks; b++) {
    		values[b] = this->values(b);
    	}

    	size_t positions[Blocks];
    	for (auto& p: positions) p = 0;

    	Int size = this->size();

    	Codec codec;

    	for (Int idx = 0; idx < size; idx++)
    	{
    		ValueT values_data[Blocks];
    		for (Int block = 0; block < Blocks; block++)
    		{
    			auto len = codec.decode(values[block], values_data[block], positions[block]);
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

    	FieldFactory<Int>::serialize(buf, this->data_sizes(), Blocks);

        for (Int block = 0; block < Blocks; block++)
        {
        	Base::template serializeSegment<IndexValueT>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
        	Base::template serializeSegment<Int>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
        	Base::template serializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);
        	FieldFactory<ValueData>::serialize(buf, this->values(block), this->data_size(block));
        }
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        auto meta = this->metadata();

    	FieldFactory<Int>::deserialize(buf, meta->size());

    	FieldFactory<Int>::deserialize(buf, this->data_sizes(), Blocks);

    	for (Int block = 0; block < Blocks; block++)
        {
        	Base::template deserializeSegment<IndexValueT>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
        	Base::template deserializeSegment<Int>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
        	Base::template deserializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);
        	FieldFactory<ValueData>::deserialize(buf, this->values(block), this->data_size(block));
        }
    }


    auto find_ge(Int block, IndexValueT value) const
    {
    	return find(block, FindGEWalker(value));
    }

    auto find_gt(Int block, IndexValueT value) const
    {
    	return find(block, FindGTWalker(value));
    }

    auto find_ge_fw(Int block, Int start, IndexValueT value) const
    {
    	return walk_fw(block, start, this->size(), FindGEWalker(value));
    }

    auto find_gt_fw(Int block, Int start, IndexValueT value) const
    {
    	return walk_fw(block, start, this->size(), FindGTWalker(value));
    }


    auto find_ge_bw(Int block, Int start, IndexValueT value) const
    {
    	return walk_bw(block, start, FindGEWalker(value));
    }

    auto find_gt_bw(Int block, Int start, IndexValueT value) const
    {
    	return walk_bw(block, start, FindGTWalker(value));
    }


    IndexValueT sum(Int block) const
    {
    	return gsum(block);
    }



    IndexValueT sum(Int block, Int end) const
    {
    	return gsum(block, end);
    }

    IndexValueT plain_sum(Int block, Int end) const
    {
    	return this->plain_gsum(block, end);
    }

    IndexValueT sum(Int block, Int start, Int end) const
    {
    	return gsum(block, start, end);
    }



    auto findGTForward(Int block, Int start, IndexValueT val) const
    {
        return this->find_gt_fw(block, start, val);
    }

    auto findGTForward(Int block, IndexValueT val) const
    {
    	return this->find_gt(block, val);
    }



    auto findGTBackward(Int block, Int start, IndexValueT val) const
    {
    	return this->find_gt_bw(block, start, val);
    }

    auto findGTBackward(Int block, IndexValueT val) const
    {
    	return this->find_gt_bw(block, this->size() - 1, val);
    }



    auto findGEForward(Int block, Int start, IndexValueT val) const
    {
    	return this->find_ge_fw(block, start, val);
    }

    auto findGEForward(Int block, IndexValueT val) const
    {
    	return this->find_ge(block, val);
    }

    auto findGEBackward(Int block, Int start, IndexValueT val) const
    {
    	return this->find_ge_bw(block, start, val);
    }

    auto findGEBackward(Int block, IndexValueT val) const
    {
    	return this->find_ge_bw(block, this->size() - 1, val);
    }


    class FindResult {
    	IndexValueT prefix_;
    	Int idx_;
    public:
    	template <typename Fn>
    	FindResult(Fn&& fn): prefix_(fn.prefix()), idx_(fn.idx()) {}

    	IndexValueT prefix() {return prefix_;}
    	Int idx() const {return idx_;}
    };

    auto findForward(SearchType search_type, Int block, Int start, IndexValueT val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findForward(SearchType search_type, Int block, IndexValueT val) const
    {
    	if (search_type == SearchType::GT)
    	{
    		return FindResult(findGTForward(block, val));
    	}
    	else {
    		return FindResult(findGEForward(block, val));
    	}
    }


    auto findBackward(SearchType search_type, Int block, Int start, IndexValueT val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, start, val));
        }
        else {
            return FindResult(findGEBackward(block, start, val));
        }
    }

    auto findBackward(SearchType search_type, Int block, IndexValueT val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, val));
        }
        else {
            return FindResult(findGEBackward(block, val));
        }
    }



    template <typename ConsumerFn>
    Int scan(Int block, Int start, Int end, ConsumerFn&& fn) const
    {
    	auto values = this->values(block);
    	TreeLayout layout = this->compute_tree_layout(this->data_size(block));
    	size_t pos = this->locate(layout, values, block, start).idx;
    	size_t data_size = this->data_size(block);

    	Codec codec;

    	Int c;
    	for (c = start; c < end && pos < data_size; c++)
    	{
    		ValueT value;
    		auto len = codec.decode(values, value, pos);
    		fn(c, value);
    		pos += len;
    	}

    	return c;
    }


    template <typename T>
    void read(Int block, Int start, Int end, T* values) const
    {
    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(start, <=, end);
    	MEMORIA_ASSERT(end, <=, this->metadata()->size());

    	scan(block, start, end, [&](Int c, auto value){
    		values[c - start] = value;
    	});
    }

    void dump_block_values(std::ostream& out = cout) const
    {
    	for (Int b = 0; b < Blocks; b++) {
    		Base::dump_block(b, out);
    	}
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

    			auto value_indexes = this->value_index(block);
    			auto size_indexes = this->size_index(block);

    			out<<"Index:"<<endl;
    			for (Int c = 0; c < index_size; c++)
    			{
    				out<<c<<": "<<value_indexes[c]<<" "<<size_indexes[c]<<std::endl;
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
    			ValueT value;
    			auto len = codec.decode(values[block], value, block_pos[block]);

    			out<<"  ("<<block_pos[block]<<") "<<value;
    			block_pos[block] += len;
    		}
    		out<<endl;
    	}
    }

};




template <
	typename IndexValueT,
	Int kBlocks,
	template <typename> class CodecT,
	typename ValueT,
	Int kBranchingFactor,
	Int kValuesPerBranch
>
struct PkdStructSizeType<PkdVQTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>> {
	static const PackedSizeType Value = PackedSizeType::VARIABLE;
};


template <
	typename IndexValueT,
	Int kBlocks,
	template <typename> class CodecT,
	typename ValueT,
	Int kBranchingFactor,
	Int kValuesPerBranch
>
struct StructSizeProvider<PkdVQTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>> {
    static const Int Value = kBlocks;
};

template <
	typename IndexValueT,
	Int kBlocks,
	template <typename> class CodecT,
	typename ValueT,
	Int kBranchingFactor,
	Int kValuesPerBranch
>
struct IndexesSize<PkdVQTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>> {
	static const Int Value = kBlocks;
};


}


#endif
