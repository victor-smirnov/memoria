
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MULTIMAP_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_MULTIMAP_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>



namespace memoria {
namespace mmap    {

using bt::IdxSearchType;
using bt::StreamTag;

template <Int StreamIdx> struct InputTupleSizeH;

template <>
struct InputTupleSizeH<0> {
	template <typename Tuple>
	static auto get(Tuple&& buffer, Int idx) -> decltype(std::get<0>(buffer[idx])[1]) {
		return std::get<0>(buffer[idx])[1];
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{
		std::get<0>(buffer[idx])[1] += value;
	}
};


template <>
struct InputTupleSizeH<1> {
	template <typename Tuple>
	static Int get(Tuple&& buffer, Int idx) {
		return 0;
	}

	template <typename Tuple, typename SizeT>
	static void add(Tuple&& buffer, Int idx, SizeT value)
	{}
};



template <typename Iterator, typename Container>
class MultimapIteratorPrefixCache: public bt::BTreeIteratorPrefixCache<Iterator, Container> {

	using Base = bt::BTreeIteratorPrefixCache<Iterator, Container>;
	using CtrSizeT = typename Container::Types::CtrSizeT;

	CtrSizeT data_size_ = 0;
	CtrSizeT data_pos_ 	= 0;

public:
	using MyType = MultimapIteratorPrefixCache<Iterator, Container>;

	CtrSizeT& data_size() {
		return data_size_;
	}

	const CtrSizeT& data_size() const {
		return data_size_;
	}

	CtrSizeT& data_pos() {
		return data_pos_;
	}

	const CtrSizeT& data_pos() const {
		return data_pos_;
	}

    bool operator==(const MyType& other) const
    {
    	return data_size_ == other.data_size_ && Base::operator==(other);
    }

    bool operator!=(const MyType& other) const
	{
    	return data_size_ != other.data_size_ || Base::operator!=(other);
    }
};

template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const MultimapIteratorPrefixCache<I, C>& cache)
{
    out<<"MitimapIteratorPrefixCache[";
    out<<"Branch prefixes: "<<cache.prefixes()<<", Leaf Prefixes: "<<cache.leaf_prefixes()<<", Size Prefixes: "<<cache.size_prefix();
    out<<", Data Size: "<<cache.data_size();
    out<<", Data Pos: "<<cache.data_pos();
    out<<"]";

    return out;
}





template <typename KeyType, Int Selector = HasFieldFactory<KeyType>::Value> struct MMapKeyStructTF;

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 1>: HasType<PkdFMTreeT<KeyType>> {};

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 0>: HasType<PkdVBMTreeT<KeyType>> {};



template <typename ValueType, Int Selector = HasFieldFactory<ValueType>::Value> struct MMapValueStructTF;

template <typename ValueType>
struct MMapValueStructTF<ValueType, 1>: HasType<PkdFSQArrayT<ValueType>> {};

template <typename ValueType>
struct MMapValueStructTF<ValueType, 0>: HasType<PkdVDArrayT<ValueType>> {};




}
}

#endif
