
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_MAP_HPP_
#define MEMORIA_PROTOTYPES_BALANCEDTREE_PAGES_MAP_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed2/packed_fse_tree.hpp>
#include <memoria/core/packed2/packed_allocator.hpp>

namespace memoria 		{
namespace balanced_tree {

namespace internl1 {

template <typename T>
struct ValueHelper {
    static void setup(IPageDataEventHandler* handler, const T& value)
    {
        handler->value("VALUE", &value);
    }
};

template <typename T>
struct ValueHelper<PageID<T> > {
    typedef PageID<T>                                                   Type;

    static void setup(IPageDataEventHandler* handler, const Type& value)
    {
        IDValue id(&value);
        handler->value("VALUE", &id);
    }
};

template <>
struct ValueHelper<EmptyValue> {
    typedef EmptyValue Type;

    static void setup(IPageDataEventHandler* handler, const Type& value)
    {
        BigInt val = 0;
        handler->value("VALUE", &val);
    }
};


}



template <typename K, typename V, Int Indexes_, typename Accum>
struct TreeMapTypes {
	typedef K 						Key;
	typedef V 						Value;
	typedef Accum					Accumulator;

	static const Int Indexes		= Indexes_;
};


template <typename Types>
class TreeMap: public PackedAllocator {
	typedef PackedAllocator 				Base;
	typedef TreeMap<Types>					MyType;
public:

	static const UInt VERSION           	= 1;

	typedef typename Types::Key				IndexKey;
	typedef typename Types::Key				Key;
	typedef typename Types::Value			Value;
	typedef typename Types::Accumulator 	Accumulator;

	static const Int Indexes				= Types::Indexes;
	static const Int Blocks					= Types::Indexes;

	typedef PackedFSETreeTypes<
			Key,Key,Key
	>										TreeTypes;

	typedef PackedFSETree<TreeTypes>		Tree;

	typedef typename MergeLists<
					typename Base::FieldsList,
		            ConstValue<UInt, VERSION>,
		            typename Tree::FieldsList,
		            IndexKey,
		            Value
    >::Result                                                                   FieldsList;

	TreeMap() {}

private:
	struct InitFn {
		Int block_size(Int items_number) const
		{
			return MyType::block_size(items_number);
		}

		Int max_elements(Int block_size)
		{
			return block_size / 4;
		}
	};

public:

	Tree* tree() {
		return Base::template get<Tree>(0);
	}

	const Tree* tree() const {
		return Base::template get<Tree>(0);
	}

	Value* values() {
		return Base::template get<Value>(1);
	}

	const Value* values() const {
		return Base::template get<Value>(1);
	}


	static Int block_size(Int tree_size)
	{
		Int tree_block_size 	= Tree::block_size(tree_size);
		Int array_block_size 	= Base::roundUpBytesToAlignmentBlocks(tree_size * sizeof(Value));

		Int client_area = tree_block_size + array_block_size;

		return Base::block_size(client_area, 2);
	}

	static Int max_tree_size(Int block_size)
	{
		return FindTotalElementsNumber2(block_size, InitFn());
	}

	void init(Int block_size)
	{
		Base::init(block_size, 2);

//		Int client_area = Base::client_area();

		Int tree_size = max_tree_size(block_size);

		Int tree_block_size = Tree::block_size(tree_size);

		Base::template allocate<Tree>(0, tree_block_size);
		Base::template allocateArrayBySize<Value>(1, tree_size);
	}


	Int findLES(Int block_num, const Key& k, Accumulator& sum) const
	{
		const Tree* tree = this->tree();

		auto result = tree->findLE(k);

		sum[0] += result.prefix();

		return result.idx();
	}

	Int size() const {
		return tree()->size();
	}

	Int& size() {
		return tree()->size();
	}

	Int maxSize() const {
		return tree()->max_size();
	}

	Accumulator keysAt(Int idx) const
	{
		Accumulator acc;

		acc[0] = tree()->value(idx);

		return acc;
	}

	Value& data(Int idx) {
		return *(values() + idx);
	}

	const Value& data(Int idx) const {
		return *(values() + idx);
	}

	Key& key(Int block_num, Int key_num)
	{
		MEMORIA_ASSERT(key_num, >=, 0);
		MEMORIA_ASSERT(key_num, <, tree()->max_size());

		Tree* tree = this->tree();
		return tree->value(block_num * tree->max_size() + key_num);
	}

	const Key& key(Int block_num, Int key_num) const
	{
		MEMORIA_ASSERT(key_num, >=, 0);
		MEMORIA_ASSERT(key_num, <, tree()->max_size());

		const Tree* tree = this->tree();
		return tree->value(block_num * tree->max_size() + key_num);
	}


	IndexKey& maxKey(Int block_num)
	{
		return tree()->indexes(block_num)[0];
	}

	const IndexKey& maxKey(Int block_num) const
	{
		return tree()->indexes(block_num)[0];
	}

	Accumulator maxKeys() const
	{
		Accumulator accum;

		accum[0] = tree()->sum();

		return accum;
	}

	void updateUp(Int block_num, Int idx, IndexKey key_value)
	{
		tree()->updateUp(block_num, idx, key_value);
	}


	void insertSpace(Int room_start, Int room_length)
	{
		MEMORIA_ASSERT(room_start, <=, size());

		Value* values = this->values();

		Int size = this->size();

		CopyBuffer(values + room_start, values + room_start + room_length, size - room_start);

		tree()->insertSpace(room_start, room_length);
	}

	void removeSpace(Int room_start, Int room_length)
	{
		Int size = this->size();

		tree()->removeSpace(room_start, room_length);

		Value* values = this->values();

		CopyBuffer(values + room_start + room_length, values + room_start, size - room_start - room_length);
	}

	void reindexAll(Int start, Int stop)
	{
		tree()->reindex();
	}

	void reindex()
	{
		tree()->reindex();
	}

	void copyTo(MyType* other, Int copy_from, Int count, Int copy_to) const
	{
		tree()->copyTo(other->tree(), copy_from, count, copy_to);

		CopyBuffer(this->values() + copy_from, other->values() + copy_to, count);
	}

	template <typename TreeType>
	void transferDataTo(TreeType* other) const
	{
		tree()->transferDataTo(other->tree());

		const auto* my_values 	= values();
		auto* other_values 		= other->values();

		Int size = this->size();

		for (Int c = 0; c < size; c++)
		{
			other_values[c] = my_values[c];
		}
	}

	void clear(Int start, Int end)
	{
		Tree* tree 		= this->tree();
		Value* values	= this->values();

		for (Int c = start; c < end; c++)
		{
			tree->value(c) 	= 0;
			values[c]		= 0;
		}
	}

	void clearUnused()
	{

	}

	void sum(Int start, Int end, Accumulator& accum) const
	{
		accum[0] += tree()->sum(start, end);
	}


	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		Base::generateDataEvents(handler);

		tree()->generateDataEvents(handler);

		handler->startGroup("TREE_VALUES", size());

		for (Int idx = 0; idx < size(); idx++)
		{
			intrnl1::ValueHelper<Value>::setup(handler, data(idx));
		}

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		Base::serialize(buf);
		tree()->serialize(buf);
		FieldFactory<Value>::serialize(buf, *values(), size());
	}

	void deserialize(DeserializationData& buf)
	{
		Base::deserialize(buf);
		tree()->deserialize(buf);
		FieldFactory<Value>::deserialize(buf, *values(), size());
	}
};



}
}


#endif
