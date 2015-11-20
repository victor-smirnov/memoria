
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CORE_PACKED_FSE_QUICK_TREE_HPP_
#define MEMORIA_CORE_PACKED_FSE_QUICK_TREE_HPP_

#include <memoria/core/packed/tree/packed_fse_quick_tree_base.hpp>


namespace memoria {

template <typename IndexValueT, Int kBlocks, typename ValueT = IndexValueT, Int kBranchingFactor = PackedTreeBranchingFactor, Int kValuesPerBranch = PackedTreeBranchingFactor>
class PkdFQTree: public PkdFQTreeBase<IndexValueT, ValueT, kBranchingFactor, kValuesPerBranch> {

	using Base 		= PkdFQTreeBase<IndexValueT, ValueT, kBranchingFactor, kValuesPerBranch>;
	using MyType 	= PkdFQTree<IndexValueT, kBlocks, ValueT, kBranchingFactor, kValuesPerBranch>;



public:
	using Base::SegmentsPerBlock;

    static constexpr UInt VERSION = 1;
    static constexpr Int Blocks = kBlocks;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
				ConstValue<UInt, Blocks>
    >;

    using Values = core::StaticVector<IndexValueT, Blocks>;

    using Metadata = typename Base::Metadata;

    using InputBuffer 	= MyType;
    using InputType 	= Values;

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




    Int data_size() const {
    	return this->size() * sizeof (ValueT) * Blocks;
    }

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
    void resize(Metadata* meta, Int size)
    {
    	Int new_data_size  = meta->max_size() + size;
    	Int new_index_size = MyType::index_size(new_data_size);

    	for (Int block = 0; block < Blocks; block++)
    	{
    		Base::resizeBlock(SegmentsPerBlock * block + 1, new_index_size * sizeof(IndexValueT));
    		Base::resizeBlock(SegmentsPerBlock * block + 2, new_data_size * sizeof(ValueT));
    	}

    	meta->max_size() 	+= size;
    	meta->index_size() 	= new_index_size;
    }

    void insertSpace(Int idx, Int room_length)
    {
    	auto meta = this->metadata();

    	Int capacity  = meta->capacity();

    	if (capacity < room_length)
    	{
    		resize(meta, room_length - capacity);
    	}

    	for (Int block = 0; block < Blocks; block++)
    	{
    		auto* values = this->values(block);

    		CopyBuffer(
    				values + idx,
					values + idx + room_length,
					meta->size() - idx
    		);

    		for (Int c = idx; c < idx + room_length; c++) {
    			values[c] = 0;
    		}
    	}

    	meta->size() += room_length;
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
    	auto meta = this->metadata();

    	Int room_length = end - start;
    	Int size = meta->size();

    	for (Int block = Blocks - 1; block >= 0; block--)
    	{
    		auto values = this->values(block);

    		CopyBuffer(
    				values + end,
					values + start,
					size - end
    		);

    		for (Int c = start + size - end; c < size; c++)
    		{
    			values[c] = 0;
    		}
    	}

    	meta->size() -= room_length;

    	resize(meta, -room_length);

    	reindex();
    }


    void insert(Int idx, Int size, std::function<Values ()> provider, bool reindex = true)
    {
    	insertSpace(idx, size);

    	typename Base::Value* values[Blocks];
    	for (Int block  = 0; block < Blocks; block++)
    	{
    		values[block] = this->values(block);
    	}

    	for (Int c = idx; c < idx + size; c++)
    	{
    		Values vals = provider();

    		for (Int block = 0; block < Blocks; block++)
    		{
    			values[block][c] = vals[block];
    		}
    	}

    	if (reindex) {
    		this->reindex();
    	}
    }

    Int insert(Int idx, std::function<bool (Values&)> provider, bool reindex = true)
    {
    	Values vals;
    	Int cnt = 0;

    	while(provider(vals) && check_capacity(1))
    	{
    		insertSpace(idx, 1);

    		for (Int block = 0; block < Blocks; block++)
    		{
    			auto values   = this->values(block);
    			values[idx] = vals[block];
    		}

    		idx++;
    		cnt++;
    	}

    	this->reindex();

    	return cnt;
    }

    template <typename T>
    void insert(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	insertSpace(idx, 1);
    	setValues(idx, values);
    }


    template <typename Adaptor>
    void _insert(Int pos, Int size, Adaptor&& adaptor)
    {
    	insertSpace(pos, size);

    	for (Int c = 0; c < size; c++)
    	{
    		auto item = adaptor(c);

    		for (Int block = 0; block < Blocks; block++)
    		{
    			this->value(block, c + pos) = item[block];
    		}
    	}

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


    BigInt setValue(Int block, Int idx, ValueT value)
    {
    	if (value != 0)
    	{
    		ValueT val = this->value(block, idx);
    		this->value(block, idx) = value;

    		return val - value;
    	}
    	else {
    		return 0;
    	}
    }



    template <typename T>
    void setValues(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	for (Int b = 0; b < Blocks; b++) {
    		this->values(b)[idx] = values[b];
    	}

    	reindex();
    }

    template <typename T>
    void addValues(Int idx, const core::StaticVector<T, Blocks>& values)
    {
    	for (Int b = 0; b < Blocks; b++) {
    		this->values(b)[idx] += values[b];
    	}

    	reindex();
    }


    void addValue(Int block, Int idx, ValueT value)
    {
    	if (value != 0)
    	{
    		this->value(block, idx) += value;
    	}

    	reindex();
    }

    template <typename T, Int Indexes>
    void addValues(Int idx, Int from, Int size, const core::StaticVector<T, Indexes>& values)
    {
    	for (Int block = 0; block < size; block++)
    	{
    		this->value(block, idx) += values[block + from];
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

template <typename IndexValueT, Int kBlocks, typename ValueT, Int kBranchingFactor, Int kValuesPerBranch>
struct PkdStructSizeType<PkdFQTree<IndexValueT, kBlocks, ValueT, kBranchingFactor, kValuesPerBranch>> {
	static const PackedSizeType Value = PackedSizeType::FIXED;
};


template <typename IndexValueT, Int kBlocks, typename ValueT, Int kBranchingFactor, Int kValuesPerBranch>
struct StructSizeProvider<PkdFQTree<IndexValueT, kBlocks, ValueT, kBranchingFactor, kValuesPerBranch>> {
    static const Int Value = kBlocks;
};

template <typename IndexValueT, Int kBlocks, typename ValueT, Int kBranchingFactor, Int kValuesPerBranch>
struct IndexesSize<PkdFQTree<IndexValueT, kBlocks, ValueT, kBranchingFactor, kValuesPerBranch>> {
	static const Int Value = kBlocks;
};


}


#endif
