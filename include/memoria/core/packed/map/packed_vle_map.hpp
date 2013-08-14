// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_VLE_MAP_HPP_
#define MEMORIA_CORE_PACKED_VLE_MAP_HPP_


#include <memoria/core/packed/tree/packed_vle_tree.hpp>
#include <memoria/core/packed/array/packed_vle_array.hpp>

#include <memoria/core/tools/exint_codec.hpp>

namespace memoria {

template <
	Int Blocks 		= 1,
	template <typename> class Codec = UByteExintCodec,

	Int BF 					= PackedTreeExintVPB,
	Int VPB 				= PackedTreeBranchingFactor
>
struct PackedVLEMapTypes: Packed2TreeTypes<BigInt, BigInt, Blocks, Codec> {};

template <typename Types>
class PackedVLEMap: public PackedAllocator {

	typedef PackedAllocator														Base;
public:
	typedef PackedVLEMap<Types>													MyType;

	enum {TREE, ARRAY};

	typedef PkdVTree<Types> 													Tree;
	typedef PackedVLEArray<Types> 												Array;

	typedef typename Types::Value												Value;
	typedef typename Tree::Values												Values;
	typedef typename Tree::Values2												Values2;

	typedef typename Tree::IndexValue											IndexValue;
	typedef typename Tree::ValueDescr											ValueDescr;

	typedef typename Array::ConstValueAccessor									ConstValueAccessor;
	typedef typename Array::ValueAccessor										ValueAccessor;

	Tree* tree()
	{
		return Base::template get<Tree>(TREE);
	}

	const Tree* tree() const
	{
		return Base::template get<Tree>(TREE);
	}

	Array* array()
	{
		return Base::template get<Array>(ARRAY);
	}

	const Array* array() const
	{
		return Base::template get<Array>(ARRAY);
	}

	Int size() const
	{
		return tree()->size();
	}

	ValueAccessor value(Int idx)
	{
		return array()->value(idx);
	}

	ConstValueAccessor value(Int idx) const
	{
		return array()->value(idx);
	}

	static Int empty_size()
	{
		Int allocator_size 		= PackedAllocator::empty_size(2);
		Int tree_empty_size 	= Tree::empty_size();
		Int array_empty_size 	= Array::empty_size();

		return allocator_size + tree_empty_size + array_empty_size;
	}

	void init()
	{
		Base::init(empty_size(), 2);

		Base::template allocateEmpty<Tree>(TREE);
		Base::template allocateEmpty<Array>(ARRAY);
	}

	void insert(Int idx, const Values& keys, const Value& value)
	{
		tree()->insert(idx, keys);

		typedef typename Array::Values ArrayValues;

		array()->insert(idx, ArrayValues(value));
	}

	void removeSpace(Int room_start, Int room_end) {
		remove(room_start, room_end);
	}

	void remove(Int room_start, Int room_end)
	{
		array()->remove(room_start, room_end);
		tree()->remove(room_start, room_end);
	}

	void splitTo(MyType* other, Int split_idx)
	{
		tree()->splitTo(other->tree(), split_idx);
		array()->splitTo(other->array(), split_idx);
	}

	void mergeWith(MyType* other)
	{
		tree()->mergeWith(other->tree());
		array()->mergeWith(other->array());
	}

	void reindex()
	{
		tree()->reindex();
		array()->reindex();
	}

	void check() const
	{
		tree()->check();
		array()->check();
	}

	void dump(std::ostream& out = std::cout) const
	{
		tree()->dump(out);
		array()->dump(out);
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
		handler->startGroup("VLE_MAP");

		Base::generateDataEvents(handler);

		tree()->generateDataEvents(handler);
		array()->generateDataEvents(handler);

		handler->endGroup();
	}

	void serialize(SerializationData& buf) const
	{
		Base::serialize(buf);

		tree()->serialize(buf);
		array()->serialize(buf);
	}

	void deserialize(DeserializationData& buf)
	{
		Base::deserialize(buf);

		tree()->deserialize(buf);
		array()->deserialize(buf);
	}

};


}

#endif
