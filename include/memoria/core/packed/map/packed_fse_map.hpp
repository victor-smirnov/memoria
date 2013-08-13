// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_FSE_MAP_HPP_
#define MEMORIA_CORE_PACKED_FSE_MAP_HPP_


#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/metadata/page.hpp>

namespace memoria {

template <
	typename Key,
	typename Value,
	Int Blocks 		= 1
>
struct PackedFSEMapTypes: Packed2TreeTypes<Key, BigInt, Blocks> {};

template <typename Types>
class PackedFSEMap: public PackedAllocator {

	typedef PackedAllocator														Base;
public:
	typedef PackedFSEMap<Types>													MyType;

	enum {TREE, ARRAY};

	typedef PkdFTree<Types> 													Tree;

	typedef typename Types::Value												Value;
	typedef typename Tree::Values												Values;
	typedef typename Tree::Values2												Values2;

	typedef typename Tree::IndexValue											IndexValue;
	typedef typename Tree::ValueDescr											ValueDescr;

	Tree* tree() {
		return Base::template get<Tree>(TREE);
	}

	const Tree* tree() const {
		return Base::template get<Tree>(TREE);
	}

	Value* values() {
		return Base::template get<Value>(ARRAY);
	}

	const Value* values() const {
		return Base::template get<Value>(ARRAY);
	}

	Int size() const
	{
		return tree()->size();
	}

	Value& value(Int idx)
	{
		return values()[idx];
	}

	const Value& value(Int idx) const
	{
		return values()[idx];
	}

	static Int empty_size()
	{
		Int allocator_size 	= PackedAllocator::empty_size(2);
		Int tree_empty_size = Tree::empty_size();

		return allocator_size + tree_empty_size;
	}

	void init()
	{
		Base::init(empty_size(), 2);

		Base::template allocateEmpty<Tree>(TREE);
		Base::template allocateArrayByLength<Value>(ARRAY, 0);
	}

	void insert(Int idx, const Values& keys, const Value& value)
	{
		Int size = this->size();

		tree()->insert(idx, keys);

		Int requested_block_size = (size + 1) * sizeof(Value);

		Base::resizeBlock(ARRAY, requested_block_size);

		Value* values = this->values();

		CopyBuffer(values + idx, values + idx + 1, size - idx);

		values[idx] = value;
	}

	void removeSpace(Int room_start, Int room_end) {
		remove(room_start, room_end);
	}

	void remove(Int room_start, Int room_end)
	{
		Int old_size = this->size();

		MEMORIA_ASSERT(room_start, >=, 0);
		MEMORIA_ASSERT(room_end, >=, room_start);
		MEMORIA_ASSERT(room_end, <=, old_size);

		Value* values = this->values();

		CopyBuffer(values + room_end, values + room_start, old_size - room_end);

		Int requested_block_size = (old_size - (room_end - room_start)) * sizeof(Value);

		Base::resizeBlock(values, requested_block_size);

		tree()->remove(room_start, room_end);
	}

	void splitTo(MyType* other, Int split_idx)
	{
		Int size = this->size();

		tree()->splitTo(other->tree(), split_idx);

		Int remainder = size - split_idx;

		other->template allocateArrayBySize<Value>(ARRAY, remainder);

		Value* other_values = other->values();
		Value* my_values 	= this->values();

		CopyBuffer(my_values + split_idx, other_values, remainder);
	}

	void mergeWith(MyType* other)
	{
		Int other_size 	= other->size();
		Int my_size 	= this->size();

		tree()->mergeWith(other->tree());

		Int other_values_block_size 		 = other->element_size(ARRAY);
		Int required_other_values_block_size = (my_size + other_size) * sizeof(Value);

		if (required_other_values_block_size >= other_values_block_size)
		{
			other->resizeBlock(ARRAY, required_other_values_block_size);
		}

		CopyBuffer(values(), other->values() + other_size, my_size);
	}

	void reindex() {
		tree()->reindex();
	}

	void check() const {
		tree()->check();
	}

	void dump(std::ostream& out = std::cout) const {
		tree()->dump(out);

		Int size = this->size();

		auto values = this->values();

		out<<"DATA"<<std::endl;

		for (Int c = 0; c < size; c++)
		{
			out<<c<<" "<<values[c]<<std::endl;
		}

		out<<std::endl;
	}


	ValueDescr findLEForward(Int block, Int start, IndexValue val) const
	{
		return this->tree()->findLEForward(block, start, val);
	}

	ValueDescr findLTForward(Int block, Int start, IndexValue val) const
	{
		return this->tree()->findLTForward(block, start, val);
	}

	void sums(Values2& values) const
	{
		tree()->sums(values);
	}

	void sums(Values& values) const
	{
		tree()->sums(values);
	}

	void sums(Int from, Int to, Values& values) const
	{
		tree()->sums(from, to, values);
	}

	void sums(Int from, Int to, Values2& values) const
	{
		tree()->sums(from, to, values);
	}

	IndexValue sum(Int block) const {
		return tree()->sum(block);
	}

	IndexValue sum(Int block, Int to) const {
		return tree()->sum(block, to);
	}

	IndexValue sum(Int block, Int from, Int to) const {
		return tree()->sum(block, from, to);
	}

	IndexValue sumWithoutLastElement(Int block) const {
		return tree()->sumWithoutLastElement(block);
	}

	void generateDataEvents(IPageDataEventHandler* handler) const
	{
		handler->startGroup("FSE_MAP");

		Base::generateDataEvents(handler);

		tree()->generateDataEvents(handler);

		handler->startGroup("DATA", size());

		auto values = this->values();

        for (Int idx = 0; idx < size(); idx++)
        {
        	vapi::ValueHelper<Value>::setup(handler, values[idx]);
        }

		handler->endGroup();

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		Base::serialize(buf);

		tree()->serialize(buf);

		FieldFactory<Value>::serialize(buf, values(), size());
	}

	void deserialize(DeserializationData& buf)
	{
		Base::deserialize(buf);

		tree()->deserialize(buf);

		FieldFactory<Value>::deserialize(buf, values(), size());
	}

};



}

#endif
