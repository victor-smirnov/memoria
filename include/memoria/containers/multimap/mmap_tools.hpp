
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MULTIMAP_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_MULTIMAP_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace mmap    {

using bt::IdxSearchType;
using bt::StreamTag;

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


template <typename K, typename V>
using MapData = std::vector<std::pair<K, std::vector<V>>>;

template <typename Data> class SizedStreamAdaptor;

template <typename Key, typename Value>
class SizedStreamAdaptor<std::vector<std::pair<Key, std::vector<Value>>>> {
	using Data = std::vector<std::pair<Key, std::vector<Value>>>;

	const Data& data_;

	using CtrSizesT = core::StaticVector<BigInt, 2>;

	const Value* values_;

public:
	SizedStreamAdaptor(const Data& data): data_(data), values_() {}

	auto prepare(StreamTag<0>) {
		return data_.size();
	}

	auto prepare(StreamTag<1>, const CtrSizesT& path)
	{
		values_ = data_[path[0]].second.data();
		return data_[path[0]].second.size();
	}

	auto buffer(StreamTag<0>, StreamTag<0>, const CtrSizesT& pos, Int block) {
		return 1;
	}

	auto buffer(StreamTag<0>, StreamTag<1>, const CtrSizesT& pos, Int block) {
		return data_[pos[0]].first;
	}

	auto buffer(StreamTag<1>, StreamTag<0>, BigInt pos, Int block) {
		return 1;
	}

	auto buffer(StreamTag<1>, StreamTag<1>, BigInt pos, Int block) {
		return values_[pos];
	}
};



namespace {

	template <Int Size, Int Idx = 0>
	struct StreamSizeAdapter {
		template <typename T, typename... Args>
		static auto process(Int stream, T&& object, Args&&... args)
		{
			if (stream == Idx)
			{
				return object.stream_size(StreamTag<Idx>(), std::forward<Args>(args)...);
			}
			else {
				return StreamSizeAdapter<Size, Idx+1>::process(stream, std::forward<T>(object), std::forward<Args>(args)...);
			}
		}
	};


	template <Int Size>
	struct StreamSizeAdapter<Size, Size> {
		template <typename T, typename... Args>
		static auto process(Int stream, T&& object, Args&&... args)
		{
			if (stream == Size)
			{
				return object.stream_size(StreamTag<Size>(), std::forward<Args>(args)...);
			}
			else {
				throw Exception(MA_RAW_SRC, SBuf() << "Invalid stream number: " << stream);
			}
		}
	};


	template <typename StreamEntry, Int Size = std::tuple_size<StreamEntry>::value - 2, Int SubstreamIdx = 0>
	struct StreamEntryTupleAdaptor;

	template <typename StreamEntry, Int Size, Int SubstreamIdx>
	struct StreamEntryTupleAdaptor {

		template <Int StreamIdx, typename EntryValue, typename T, typename... Args>
		static void process_value(const StreamTag<StreamIdx>& tag, EntryValue&& value, T&& object, Args&&... args)
		{
			value = object.buffer(tag, StreamTag<SubstreamIdx>(), std::forward<Args>(args)..., 0);
		}

		template <Int StreamIdx, typename V, Int Indexes, typename T, typename... Args>
		static void process_value(const StreamTag<StreamIdx>& tag, core::StaticVector<V, Indexes>& value, T&& object, Args&&... args)
		{
			for (Int i = 0; i < Indexes; i++)
			{
				value[i] = object.buffer(tag, StreamTag<SubstreamIdx>(), std::forward<Args>(args)..., i);
			}
		}


		template <Int StreamIdx, typename T, typename... Args>
		static void process(const StreamTag<StreamIdx>& tag, StreamEntry& entry, T&& object, Args&&... args)
		{
			process_value(tag, get<SubstreamIdx>(entry), std::forward<T>(object), std::forward<Args>(args)...);

			StreamEntryTupleAdaptor<StreamEntry, Size, SubstreamIdx + 1>::process(tag, entry, std::forward<T>(object), std::forward<Args>(args)...);
		}
	};

	template <typename StreamEntry, Int Size>
	struct StreamEntryTupleAdaptor<StreamEntry, Size, Size> {
		template <Int StreamIdx, typename EntryValue, typename T, typename... Args>
		static void process_value(const StreamTag<StreamIdx>& tag, EntryValue&& value, T&& object, Args&&... args)
		{
			value = object.buffer(tag, StreamTag<Size>(), std::forward<Args>(args)..., 0);
		}

		template <Int StreamIdx, typename V, Int Indexes, typename T, typename... Args>
		static void process_value(const StreamTag<StreamIdx>& tag, core::StaticVector<V, Indexes>& value, T&& object, Args&&... args)
		{
			for (Int i = 0; i < Indexes; i++)
			{
				value[i] = object.buffer(tag, StreamTag<Size>(), std::forward<Args>(args)..., i);
			}
		}


		template <Int StreamIdx, typename T, typename... Args>
		static void process(const StreamTag<StreamIdx>& tag, StreamEntry& entry, T&& object, Args&&... args)
		{
			process_value(tag, get<Size>(entry), std::forward<T>(object), std::forward<Args>(args)...);
		}
	};
}





template <typename CtrT, typename Data, Int LevelStart = 0> class MMapStreamingAdapter;


template <typename CtrT, typename Data, Int LevelStart>
class MMapStreamingAdapter {

public:
	using CtrSizesT 	= typename CtrT::Types::Position;
	using CtrSizeT 		= typename CtrT::CtrSizeT;

	template <Int StreamIdx>
	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<StreamIdx>;

	template <Int StreamIdx>
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

	static constexpr Int Streams = CtrT::Types::Streams;

	using LastStreamEntry = typename CtrT::Types::template StreamInputTuple<Streams - 1>;

private:
	Data& data_;

	CtrSizesT counts_;
	CtrSizesT current_limits_;
	CtrSizesT totals_;

	Int level_ = 0;

	StreamEntryBuffer<LastStreamEntry> last_stream_buf_;

	template <Int, Int> friend struct StreamSizeAdapter;

public:
	MMapStreamingAdapter(Data& data):
		data_(data),
		level_(LevelStart)
	{
		current_limits_[level_] = StreamSizeAdapter<Streams - 1>::process(level_, *this, counts_);
	}

	auto query()
	{
		if (counts_[level_] < current_limits_[level_])
		{
			if (level_ < Streams - 1)
			{
				level_++;

				counts_[level_] = 0;
				current_limits_[level_] = StreamSizeAdapter<Streams - 1>::process(level_, *this, counts_);

				return bttl::RunDescr(level_ - 1, 1);
			}
			else {
				return bttl::RunDescr(level_, current_limits_[level_] - counts_[level_]);
			}
		}
		else if (level_ > 0)
		{
			level_--;
			return this->query();
		}
		else {
			return bttl::RunDescr(-1);
		}
	}

	template <Int StreamIdx, typename Buffer>
	auto populate(const StreamTag<StreamIdx>& tag, Buffer&& buffer, CtrSizeT length)
	{
		for (auto c = 0; c < length; c++)
		{
			StreamEntryTupleAdaptor<InputTuple<StreamIdx>>::process(tag, buffer.append_entry(), data_, counts_);

			counts_[StreamIdx] += 1;
			totals_[StreamIdx] += 1;
		}

		return length;
	}

	template <typename Buffer>
	auto populateLastStream(Buffer&& buffer, CtrSizeT length)
	{
		constexpr Int Level = Streams - 1;

		Int inserted = buffer.buffer()->append_vbuffer(&data_, counts_[Level], length);

		counts_[Level] += inserted;
		totals_[Level] += inserted;

		return inserted;
	}

	const CtrSizesT& consumed() const{
		return totals_;
	}

private:

	auto stream_size(StreamTag<0>, const CtrSizesT& pos)
	{
		return data_.prepare(StreamTag<0>());
	}

	template <Int StreamIdx>
	auto stream_size(StreamTag<StreamIdx>, const CtrSizesT& pos)
	{
		return data_.prepare(StreamTag<StreamIdx>(), pos);
	}
};






}
}

#endif
