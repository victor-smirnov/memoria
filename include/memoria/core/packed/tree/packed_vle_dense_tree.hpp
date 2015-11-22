
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_VLE_DENSE_TREE_HPP_
#define MEMORIA_CORE_PACKED_VLE_DENSE_TREE_HPP_

#include <memoria/core/packed/tree/packed_vle_quick_tree_base.hpp>


namespace memoria {

template <typename Codec>
struct PkdVDTreeShapeProvider {
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
	Int kBranchingFactor = PkdVDTreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
	Int kValuesPerBranch = PkdVDTreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
class PkdVDTree: public PkdVQTreeBase<IndexValueT, ValueT, CodecT, kBranchingFactor, kValuesPerBranch> {

	using Base 		= PkdVQTreeBase<IndexValueT, ValueT, CodecT, kBranchingFactor, kValuesPerBranch>;
	using MyType 	= PkdVDTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>;

public:
    using Base::BlocksStart;
    using Base::SegmentsPerBlock;
    using Base::compute_tree_layout;
    using Base::gsum;
    using Base::find;
    using Base::walk_fw;
    using Base::walk_bw;
    using Base::metadata;

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::VALUE_INDEX;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::Metadata;
    using typename Base::TreeLayout;
    using typename Base::OffsetsType;
    using typename Base::ValueData;
    using typename Base::FindGEWalker;
    using typename Base::FindGTWalker;


    using typename Base::Codec;

    static constexpr UInt VERSION = 1;
    static constexpr Int TreeBlocks = 1;
    static constexpr Int Blocks = kBlocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<UInt, TreeBlocks>,
    			ConstValue<UInt, Blocks>
    >;

    using Values = core::StaticVector<IndexValueT, Blocks>;

    using InputBuffer 	= MyType;
    using InputType 	= Values;

    using SizesT = core::StaticVector<Int, Blocks>;

    void init(Int data_block_size)
    {
    	Base::init(data_block_size, TreeBlocks);
    }

    void init_tl(Int data_block_size)
    {
    	Base::init_tl(data_block_size, TreeBlocks);
    }

    void init()
    {
    	Base::init(0, TreeBlocks);
    }

    static Int block_size(Int capacity)
    {
    	return Base::block_size(TreeBlocks, capacity * Blocks);
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
        return block_size(this->size() + other->size());
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

    static Int elements_for(Int block_size)
    {
        return Base::tree_size(TreeBlocks, block_size);
    }

    static Int expected_block_size(Int items_num)
    {
        return block_size(items_num);
    }

    ValueT value(Int block, Int idx) const
    {
    	Int size = this->size();

    	if (idx >= size) {
    		this->dump();
    	}

    	MEMORIA_ASSERT(idx, >=, 0);
    	MEMORIA_ASSERT(idx, <, size);

    	Int data_size	  = this->data_size();
    	auto values 	  = this->values();
    	TreeLayout layout = this->compute_tree_layout(data_size);

    	Int global_idx = idx + size * block;
		Int start_pos  	  = this->locate(layout, values, 0, global_idx).idx;

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
    void resize_segments(Int new_data_size)
    {
    	Int block = 0;

    	Int data_segment_size 	 = PackedAllocator::roundUpBitsToAlignmentBlocks(new_data_size * Codec::ElementSize);
    	Int index_size 	 	   	 = Base::index_size(new_data_size);
    	Int offsets_segment_size = Base::offsets_segment_size(new_data_size);

    	this->resizeBlock(block * SegmentsPerBlock + VALUES + BlocksStart, data_segment_size);
    	this->resizeBlock(block * SegmentsPerBlock + OFFSETS + BlocksStart, offsets_segment_size);
    	this->resizeBlock(block * SegmentsPerBlock + SIZE_INDEX + BlocksStart, index_size * sizeof(Int));
    	this->resizeBlock(block * SegmentsPerBlock + VALUE_INDEX + BlocksStart, index_size * sizeof(IndexValueT));
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


//    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
//    {
//    	MEMORIA_ASSERT_TRUE(copy_from >= 0);
//    	MEMORIA_ASSERT_TRUE(count >= 0);
//
//    	for (Int block = 0; block < Blocks; block++)
//    	{
//    		auto my_values 	  = this->values(block);
//    		auto other_values = other->values(block);
//
//    		CopyBuffer(
//    				my_values + copy_from,
//					other_values + copy_to,
//					count
//    		);
//    	}
//    }

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
    		Int& data_size	= this->data_size();
    		auto values			= this->values();
    		TreeLayout layout 	= compute_tree_layout(data_size);
    		Int size			= this->size();

    		Codec codec;

    		Int start_pos[Blocks];
    		Int lengths[Blocks];

    		for (Int block = 0; block < Blocks; block++)
    		{
    			start_pos[block] = this->locate(layout, values, 0, start + size * block).idx;
    			Int end_pos 	 = this->locate(layout, values, 0, end + size * block).idx;

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
    	Codec codec;

    	SizesT total_lengths;
    	Int total_length = 0;

    	Int positions[Blocks];
    	Int size 	  = this->size();
    	Int& data_size = this->data_size();

    	TreeLayout layout = compute_tree_layout(data_size);

    	auto values	= this->values();

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int global_idx   = size * block + idx;
    		positions[block] = this->locate(layout, values, 0, global_idx).idx;

    		for (SizeT c = 0; c < inserted; c++)
    		{
    			auto value = adaptor(c)[block];
    			auto len = codec.length(value);

    			total_lengths[block] += len;
    		}

    		total_length += total_lengths[block];
    	}


    	resize_segments(data_size + total_length);

    	values = this->values();

    	for (Int block = 0; block < Blocks; block++)
    	{
    		size_t insertion_pos = positions[block];
    		codec.move(values, insertion_pos, insertion_pos + total_lengths[block], data_size - insertion_pos);

    		for (Int c = 0; c < inserted; c++)
    		{
    			auto value = adaptor(c)[block];
    			Int len = codec.encode(values, value, insertion_pos);
    			insertion_pos += len;
    		}

    		for (Int b1 = block + 1; b1 < Blocks; b1++) {
    			positions[b1] += total_lengths[block];
    		}

    		data_size += total_lengths[block];
    	}

    	metadata()->size() += (inserted * Blocks);

    	reindex();
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


    	for (Int block = 0; block < Blocks; block++)
    	{
    		Int total_delta = 0;

    		for (Int window_start = start; window_start < end; window_start += 32)
    		{
    			Int window_end = (window_start + 32) < end ? window_start + 32 : end;

    			Int old_length = 0;
    			Int new_length = 0;

    			size_t data_start_tmp = starts[block];

    			ValueT buffer[32];

    			for (Int c = window_start; c < window_end; c++)
    			{
    				ValueT old_value;
    				auto len = codec.decode(values, old_value, data_start_tmp);

    				auto new_value = update_fn(block, c, old_value);

    				buffer[c - window_start] = new_value;

    				old_length += len;
    				new_length += codec.length(new_value);

    				data_start_tmp += len;
    			}

    			size_t data_start = starts[block];

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

    		for (Int b1 = block; b1 < Blocks; b1++) {
    			starts[b1] += total_delta;
    		}
    	}

    	reindex();
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
    	Int size 	   = this->size();

    	MEMORIA_ASSERT(start, >=, 0);
    	MEMORIA_ASSERT(start, <, size);

    	Int global_idx = block * size + start;

    	Codec codec;

    	Int data_size       = this->data_size();
    	auto values			= this->values();
    	TreeLayout layout 	= compute_tree_layout(data_size);

    	size_t insertion_pos = this->locate(layout, values, 0, global_idx).idx;

    	ValueT value;
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
    	handler->value("DATA_SIZE", this->data_sizes(), TreeBlocks);

    	handler->startGroup("INDEXES", TreeBlocks);

    	for (Int block = 0; block < TreeBlocks; block++)
    	{
    		Int index_size = this->index_size(Base::data_size(block));

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

    	Int size = this->size();

    	Codec codec;

    	size_t positions[Blocks];
    	for (Int block = 0; block < Blocks; block++) {
    		positions[block] = this->locate(0, block * size);
    	}

    	auto values = this->values();

    	for (Int idx = 0; idx < size; idx++)
    	{
    		ValueT values_data[Blocks];
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

    	FieldFactory<Int>::serialize(buf, this->data_sizes(), TreeBlocks);

        for (Int block = 0; block < TreeBlocks; block++)
        {
        	Base::template serializeSegment<IndexValueT>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
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

    	FieldFactory<Int>::deserialize(buf, this->data_sizes(), TreeBlocks);

    	for (Int block = 0; block < TreeBlocks; block++)
        {
        	Base::template deserializeSegment<IndexValueT>(buf, block * SegmentsPerBlock + BlocksStart + VALUE_INDEX);
        	Base::template deserializeSegment<Int>(buf, block * SegmentsPerBlock + BlocksStart + SIZE_INDEX);
        	Base::template deserializeSegment<OffsetsType>(buf, block * SegmentsPerBlock + BlocksStart + OFFSETS);
        	FieldFactory<ValueData>::deserialize(buf, Base::values(block), Base::data_size(block));
        }
    }


    auto find_ge(Int block, IndexValueT value) const
    {
    	Int size = this->size();
    	Int block_start = block * size;
    	return walk_fw(0, block_start, FindGEWalker(value)).adjust(block_start, size);
    }

    auto find_gt(Int block, IndexValueT value) const
    {
    	Int size = this->size();
    	Int block_start = block * size;
    	return find(0, FindGTWalker(value)).adjust(block_start, size);
    }

    auto find_ge_fw(Int block, Int start, IndexValueT value) const
    {
    	Int size = this->size();
    	Int block_start = block * size;
    	return walk_fw(0, block_start + start, FindGEWalker(value)).adjust(block_start, size);
    }

    auto find_gt_fw(Int block, Int start, IndexValueT value) const
    {
    	Int size = this->size();
    	Int block_start = block * size;
    	return walk_fw(0, block_start + start, FindGTWalker(value)).adjust(block_start, size);
    }


    auto find_ge_bw(Int block, Int start, IndexValueT value) const
    {
    	Int size = this->size();
    	Int block_start = block * size;
    	return walk_bw(0, block_start + start, FindGEWalker(value)).adjust(block_start, size);
    }

    auto find_gt_bw(Int block, Int start, IndexValueT value) const
    {
    	Int size = this->size();
    	Int block_start = block * size;
    	return walk_bw(0, block_start + start, FindGTWalker(value)).adjust(block_start, size);
    }


    IndexValueT sum(Int block) const
    {
    	Int size = this->size();
    	return gsum(0, size * block, size * block + size);
    }



    IndexValueT sum(Int block, Int end) const
    {
    	Int size = this->size();
    	return gsum(0, size * block, size * block + end);
    }

    IndexValueT plain_sum(Int block, Int end) const
    {
    	Int size = this->size();
    	return this->plain_gsum(0, size * block + end) - this->plain_gsum(0, size * block);
    }

    IndexValueT sum(Int block, Int start, Int end) const
    {
    	Int size = this->size();
    	return gsum(0, size * block + start, size * block + end);
    }



    auto findGTForward(Int block, Int start, IndexValueT val) const
    {
        return this->find_gt_fw(block, start, val);
    }



    auto findGTBackward(Int block, Int start, IndexValueT val) const
    {
    	return this->find_gt_bw(block, start, val);
    }



    auto findGEForward(Int block, Int start, IndexValueT val) const
    {
    	return this->find_ge_fw(block, start, val);
    }

    auto findGEBackward(Int block, Int start, IndexValueT val) const
    {
    	return this->find_ge_bw(block, start, val);
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

    	size_t block_starts[Blocks];

    	for (Int block = 0; block < Blocks; block++)
    	{
    		block_starts[block] = this->locate(0, block * size);
    		out<<"block_data_size_["<<block<<"] = "<<block_starts[block]<<std::endl;
    	}

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

    		auto value_indexes = this->value_index(0);
    		auto size_indexes = this->size_index(0);

    		out<<"Index:"<<endl;
    		for (Int c = 0; c < index_size; c++)
    		{
    			out<<c<<": "<<value_indexes[c]<<" "<<size_indexes[c]<<std::endl;
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

    	for (Int c = 0; c < size; c++)
    	{
    		out<<"c: "<<c<<" ";
    		for (Int block = 0; block < Blocks; block++)
    		{
    			ValueT value;
    			auto len = codec.decode(values, value, block_starts[block]);
    			out<<" ("<<block_starts[block]<<") "<<value<<" ";
    			block_starts[block] += len;
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
struct PkdStructSizeType<PkdVDTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>> {
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
struct StructSizeProvider<PkdVDTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>> {
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
struct IndexesSize<PkdVDTree<IndexValueT, kBlocks, CodecT, ValueT, kBranchingFactor, kValuesPerBranch>> {
	static const Int Value = kBlocks;
};


}


#endif
