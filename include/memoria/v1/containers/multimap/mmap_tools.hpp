
// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/tools/ticker.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace v1 {
namespace mmap {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename KeyType, int32_t Selector = HasFieldFactory<KeyType>::Value> struct MMapKeyStructTF;
template <typename KeyType, int32_t Selector = HasFieldFactory<KeyType>::Value> struct MMapSumKeyStructTF;

template <typename T> struct MMapBranchStructTF;

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 1>: HasType<PkdFMTreeT<KeyType>> {};

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 0>: HasType<PkdVBMTreeT<KeyType>> {};


template <typename KeyType>
struct MMapSumKeyStructTF<KeyType, 1>: HasType<PkdFQTreeT<KeyType>> {};

template <typename KeyType>
struct MMapSumKeyStructTF<KeyType, 0>: HasType<PkdVQTreeT<KeyType>> {};


template <typename ValueType, int32_t Selector = HasFieldFactory<ValueType>::Value> struct MMapValueStructTF;

template <typename ValueType>
struct MMapValueStructTF<ValueType, 1>: HasType<PkdFSQArrayT<ValueType>> {};

template <typename ValueType>
struct MMapValueStructTF<ValueType, 0>: HasType<PkdVDArrayT<ValueType>> {};







template <typename KeyType>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};


template <typename KeyType>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, int32_t Indexes>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
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

template <typename KeyType, int32_t Indexes>
struct MMapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    using Type = IfThenElse<
            HasFieldFactory<KeyType>::Value,
            PkdFMOTreeT<KeyType, Indexes>,
            PkdVBMTreeT<KeyType>
    >;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};









template <typename IOBuffer, typename Key, typename Iterator>
class MultimapEntryBufferProducer: public bt::BufferProducer<IOBuffer> {

	Key key_;
	Iterator start_;
	Iterator end_;

	bool key_finished_ = false;

	using Value = typename Iterator::value_type;

	static constexpr int32_t DataStreams = 2;

	static constexpr int32_t KeyStream 		= 0;
	static constexpr int32_t ValueStream 	= 1;

	static constexpr size_t ValueBlockSize = 256;

public:
	MultimapEntryBufferProducer(const Key& key, const Iterator& start, const Iterator& end):
		key_(key), start_(start), end_(end)
	{}

	virtual int32_t populate(IOBuffer& buffer)
	{
		int32_t entries = 0;

		if (!key_finished_)
		{
			if (!buffer.template putSymbolsRun<DataStreams>(KeyStream, 1))
			{
				throw Exception(MA_SRC, "Supplied IOBuffer is probably too small");
			}

			if (!IOBufferAdapter<Key>::put(buffer, key_))
			{
				throw Exception(MA_SRC, "Supplied IOBuffer is probably too small");
			}

			key_finished_ = true;
			entries += 2;
		}

		while (start_ != end_)
		{
			size_t pos = buffer.pos();
			if (!buffer.template putSymbolsRun<DataStreams>(ValueStream, ValueBlockSize))
			{
				return entries;
			}

			entries++;

			size_t c;
			for (c = 0; c < ValueBlockSize && start_ != end_; c++, entries++, start_++)
			{
				if (!IOBufferAdapter<Value>::put(buffer, *start_))
				{
					if (c > 0) {
						buffer.template updateSymbolsRun<DataStreams>(pos, ValueStream, c);
					}
					else {
						entries--;
					}

					return entries;
				}
			}

			if (c < ValueBlockSize)
			{
				buffer.template updateSymbolsRun<DataStreams>(pos, ValueStream, c);
			}
		}

		return -entries;
	}
};






}
}}
