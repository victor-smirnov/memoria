
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAPX_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_MAPX_CONTAINER_TOOLS_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/core/packed/array/packed_fse_array.hpp>
#include <memoria/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/prototypes/bt_ss/btss_input.hpp>

namespace memoria       {
namespace map           {

using bt::IdxSearchType;


template <typename KeyType, Int Selector> struct MapKeyStructTF;

template <typename KeyType>
struct MapKeyStructTF<KeyType, 1>: HasType<PkdFMTreeT<KeyType>> {};

template <typename KeyType>
struct MapKeyStructTF<KeyType, 0>: HasType<PkdVBMTreeT<KeyType>> {};



template <typename ValueType, Int Selector> struct MapValueStructTF;

template <typename ValueType>
struct MapValueStructTF<ValueType, 1>: HasType<PkdFSQArrayT<ValueType>> {};

template <typename ValueType>
struct MapValueStructTF<ValueType, 0>: HasType<PkdVDArrayT<ValueType>> {};




template <typename T> struct MapBranchStructTF;

template <typename KeyType>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
	using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};

template <typename KeyType>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
	using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, Int Indexes>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
{
	static_assert(
			IsExternalizable<KeyType>::Value,
			"Type must either has ValueCodec or FieldFactory defined"
	);

	//FIXME: Extend KeyType to contain enough space to represent practically large sums
	//Should be done systematically on the level of BT

	using Type = IfThenElse <
			HasFieldFactory<KeyType>::Value,
			PkdFQTreeT<KeyType, Indexes>,
			PkdVQTreeT<KeyType, Indexes>
	>;

	static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, Int Indexes>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

	static_assert(
			IsExternalizable<KeyType>::Value,
			"Type must either has ValueCodec or FieldFactory defined"
	);

	using Type = IfThenElse<
			HasFieldFactory<KeyType>::Value,
			PkdFMTreeT<KeyType, Indexes>,
			PkdVBMTreeT<KeyType>
	>;

	static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};




template <typename CtrT, typename EntryProvider, Int EntryBufferSize = 1000>
class MapEntryInputProvider: public memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
	using Base = memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

	using typename Base::CtrSizeT;
	using typename Base::Position;
	using typename Base::InputBuffer;

	using Entry	= typename CtrT::Types::Entry;

	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;

	EntryProvider provider_;

	Int input_start_ = 0;
	Int input_size_ = 0;
	static constexpr Int INPUT_END = EntryBufferSize;

	Entry input_value_buffer_[INPUT_END];

	BigInt zero_ = 0;

public:
	MapEntryInputProvider(CtrT& ctr, Int capacity = 10000):
		Base(ctr, capacity)
	{}

	MapEntryInputProvider(CtrT& ctr, const EntryProvider& provider, Int capacity = 10000):
		Base(ctr, capacity),
		provider_(provider)
	{}

	virtual Int get(InputBuffer* buffer, Int pos)
	{
		if (input_start_ == input_size_)
		{
			input_start_ = 0;

			for (input_size_ = 0 ; provider_.has_next() && input_size_ < INPUT_END; input_size_++)
			{
				input_value_buffer_[input_size_] = provider_.next();
			}
		}

		if (input_start_ < input_size_)
		{
			auto inserted = buffer->append_vbuffer(this, input_start_, input_size_ - input_start_);
			input_start_ += inserted;

			return inserted;
		}

		return -1;
	}

	const auto& buffer(bt::StreamTag<0>, bt::StreamTag<0>, Int idx, Int block) {
		return zero_;
	}

	const auto& buffer(bt::StreamTag<0>, bt::StreamTag<1>, Int idx, Int block) {
		return std::get<0>(input_value_buffer_[idx]);
	}

	const auto& buffer(bt::StreamTag<0>, bt::StreamTag<2>, Int idx, Int block) {
		return std::get<1>(input_value_buffer_[idx]);
	}
};




}
}

#endif
