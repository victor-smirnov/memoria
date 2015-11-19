
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
	typename ValueT = IndexValueT,
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

    using Base::METADATA;
    using Base::DATA_SIZES;

    using Base::VALUES;
    using Base::VALUE_INDEX;
    using Base::OFFSETS;
    using Base::SIZE_INDEX;

    using typename Base::Metadata;
    using typename Base::TreeLayout;


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

    void init(Int data_block_size)
    {
    	Base::init(data_block_size, Blocks);
    }

    void init_tl(Int data_block_size)
    {
    	Base::init_tl(data_block_size, Blocks);
    }

    void init()
    {
    	Base::init(0, Blocks);
    }

    static Int block_size(Int capacity)
    {
    	return Base::block_size(Blocks, capacity);
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
    	Int data_size	  = this->data_size(block);
    	auto values 	  = this->values(block);
    	TreeLayout layout = this->compute_tree_layout(data_size);
		Int start_pos  	  = this->locate(layout, values, block, idx);

		Codec codec;
		ValueT value;

		codec.decode(values, value, start_pos);

		return value;
    }


//    Int data_size() const {
//    	return this->size() * sizeof (ValueT) * Blocks;
//    }

    static Int empty_size()
    {
    	return block_size(0);
    }

    void reindex() {
    	Base::reindex(Blocks);
    }

    void dump_index(std::ostream& out = cout) const {
    	Base::dump_index(Blocks, out);
    }

    void dump(std::ostream& out = cout) const {
    	Base::dump(Blocks, out);
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



    template <typename... Args>
    auto sum(Args&&... args) const -> decltype(Base::sum(std::forward<Args>(args)...)) {
    	return Base::sum(std::forward<Args>(args)...);
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


    auto insert_space(Int block, Int start, Int length)
    {
    	Int data_size = this->data_size(block);
    	resize(block, data_size, start, length);

    	auto values = this->values(block);

    	Codec codec;
    	codec.move(values, start, start + length, data_size - start);

    	this->data_size(block) += length;

    	return values;
    }

    auto remove_space(Int block, Int start, Int end)
    {
    	Int data_size = this->data_size(block);
    	resize(block, data_size, start, end - start);

    	auto values = this->values(block);

    	Codec codec;
    	codec.move(values, end, start, data_size - end);

    	this->data_size(block) -= (end - start);

    	return values;
    }


    void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
    {
    	MEMORIA_ASSERT_TRUE(copy_from >= 0);
    	MEMORIA_ASSERT_TRUE(count >= 0);

    	for (Int block = 0; block < Blocks; block++)
    	{
    		auto my_values 	  = this->values(block);
    		auto other_values = other->values(block);

    		CopyBuffer(
    				my_values + copy_from,
					other_values + copy_to,
					count
    		);
    	}
    }

public:
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
    	other->insertSpace(0, this->size());

    	Int size = this->size();

    	for (Int block = 0; block < Blocks; block++)
    	{
    		const auto* my_values   = this->values(block);
    		auto* other_values      = other->values(block);

    		for (Int c = 0; c < size; c++)
    		{
    			other_values[c] = my_values[c];
    		}
    	}

    	other->reindex();
    }


    void removeSpace(Int start, Int end)
    {
    	remove(start, end);
    }

    void remove(Int start, Int end)
    {
    	for (Int block = 0; block < Blocks; block++)
    	{
    		const Int data_size	= this->data_size(block);
    		auto values			= this->values(block);
    		TreeLayout layout 	= compute_tree_layout(data_size);

    		size_t start_pos = this->locate(layout, values, block, start);
    		size_t end_pos   = this->locate(layout, values, block, end);

    		this->remove_space(block, start_pos, end_pos);
    	}

    	this->size() -= end - start;

    	reindex();
    }




    template <typename T>
    void insert(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	insert_space(idx, 1);
    	setValues(idx, values);
    }


    template <typename Adaptor>
    void _insert(Int pos, Int processed, Adaptor&& adaptor)
    {
    	Codec codec;

    	SizesT total_lengths;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		for (SizeT c = pos; c < pos + processed; c++)
    		{
    			auto value = adaptor(c)[block];
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

    		data_size = this->data_size(block);
    		values	  = this->values(block);

    		for (Int c = pos; c < pos + processed; c++)
    		{
    			auto value = adaptor(c)[block];
    			Int len = codec.encode(values, value, insertion_pos, data_size);
    			insertion_pos += len;
    		}
    	}

    	this->size() += processed;

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
    	Codec codec;

    	for (Int block = 0; block < Blocks; block++)
    	{
    		auto values			= this->values(block);
    		size_t data_size 	= this->data_size(block);
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
        			data_size = this->data_size(block);
    			}
    			else if (new_length < old_length)
    			{
    				auto delta = old_length - new_length;
    				this->remove_space(block, data_start, delta);

        			values = this->values(block);
        			data_size = this->data_size(block);
    			}

    			for (Int c = window_start; c < window_end; c++)
    			{
    				data_start += codec.encode(values, buffer[c], data_start, data_size);
    			}
    		}

    		this->reindex_block(block);
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
    	Codec codec;

    	size_t data_size = this->data_size(block);

    	auto values			= this->values(block);
    	TreeLayout layout 	= compute_tree_layout(data_size);
    	size_t insertion_pos = this->locate(layout, values, block, start);

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
    			data_size = this->data_size(block);

    		}
    		else if (old_length > new_length)
    		{
    			this->remove_space(block, insertion_pos, old_length - new_length);
    			values = this->values(block);
    			data_size = this->data_size(block);
    		}

    		codec.encode(values, new_value, insertion_pos, data_size);

    		this->reindex_block(block);
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




    void check() const {}

    void clear()
    {
        init();

        if (Base::has_allocator())
        {
            auto alloc = this->allocator();
            Int empty_size = MyType::empty_size();
            alloc->resizeBlock(this, empty_size);
        }
    }

    void clear(Int start, Int end)
    {
        for (Int block = 0; block < Blocks; block++)
        {
            auto values = this->values(block);

            for (Int c = start; c < end; c++)
            {
                values[c] = 0;
            }
        }
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::generateDataEvents(handler);

    	handler->startStruct();
    	handler->startGroup("FSE_TREE");

    	auto meta = this->metadata();

    	handler->value("SIZE",          &meta->size());
    	handler->value("MAX_SIZE",      &meta->max_size());
    	handler->value("INDEX_SIZE",    &meta->index_size());

    	handler->startGroup("INDEXES", meta->index_size());

    	auto index_size = meta->index_size();

    	const IndexValueT* index[Blocks];

    	for (Int b = 0; b < Blocks; b++) {
    		index[b] = this->index(b);
    	}

    	for (Int c = 0; c < index_size; c++)
    	{
    		IndexValueT indexes[Blocks];
    		for (Int block = 0; block < Blocks; block++)
    		{
    			indexes[block] = index[block][c];
    		}

    		handler->value("INDEX", indexes, Blocks);
    	}

    	handler->endGroup();

    	handler->startGroup("DATA", meta->size());



    	const ValueT* values[Blocks];

    	for (Int b = 0; b < Blocks; b++) {
    		values[b] = this->values(b);
    	}

    	for (Int idx = 0; idx < meta->size() ; idx++)
    	{
    		ValueT values_data[Blocks];
    		for (Int block = 0; block < Blocks; block++)
    		{
    			values_data[block] = values[block][idx];
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

    	const Metadata* meta = this->metadata();

    	FieldFactory<Int>::serialize(buf, meta->size());
        FieldFactory<Int>::serialize(buf, meta->max_size());
        FieldFactory<Int>::serialize(buf, meta->index_size());

        for (Int b = 0; b < Blocks; b++)
        {
        	FieldFactory<IndexValueT>::serialize(buf, this->index(b), meta->index_size());
        	FieldFactory<ValueT>::serialize(buf, this->values(b), meta->size());
        }
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<Int>::deserialize(buf, meta->size());
        FieldFactory<Int>::deserialize(buf, meta->max_size());
        FieldFactory<Int>::deserialize(buf, meta->index_size());

        for (Int b = 0; b < Blocks; b++)
        {
        	FieldFactory<IndexValueT>::deserialize(buf, this->index(b), meta->index_size());
        	FieldFactory<ValueT>::deserialize(buf, this->values(b), meta->size());
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
	static const PackedSizeType Value = PackedSizeType::FIXED;
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
