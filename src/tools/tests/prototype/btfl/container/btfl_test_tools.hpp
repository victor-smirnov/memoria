
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_factory.hpp>


#include <functional>
#include <vector>
#include <algorithm>

namespace memoria {
namespace v1 {
namespace btfl_test {

namespace {

    struct PairCompare {
        template <typename K, typename V>
        bool operator()(const std::tuple<K, V>& a, const std::tuple<K, V>& b)
        {
            return std::get<0>(a) < std::get<0>(b);
        }
    };
}




template <Int DataStreams, Int Level = DataStreams - 1, typename KeyType = BigInt, typename ValueType = UByte, typename ColumnType = BigInt>
using BTFLData = btfl::BTFLData<DataStreams, Level, KeyType, ValueType, ColumnType>;



template <typename N, typename T>
T fromNumber(N&& value, const TypeTag<T>&) {
    return value;
}



template <typename DataSetType, Int Stream = 0, Int DataStreamsMax = btfl::BTFLDataStreamsCounter<DataSetType>::Value - 1>
struct BTFLDataSetBuilder;

template <typename K, typename V, Int Stream, Int DataStreamsMax, template <typename...> class Container, typename... Args>
struct BTFLDataSetBuilder<Container<std::tuple<K, V>, Args...>, Stream, DataStreamsMax> {

    template <typename SizeT, Int Indexes, typename Rng>
    static auto build(const core::StaticVector<SizeT, Indexes>& level_sizes, Rng&& rng, bool sort)
    {
        Container<std::tuple<K, V>, Args...> data;

        SizeT level_size = Stream == 0 ? level_sizes[Stream] : rng(level_sizes[Stream]);

        for (SizeT c = 0; c < level_size; c++)
        {
            auto value = BTFLDataSetBuilder<V, Stream + 1, DataStreamsMax>::build(level_sizes, std::forward<Rng>(rng), sort);
            data.emplace_back(
                std::make_pair(fromNumber(c, TypeTag<K>()), value)
            );
        }

        if (sort) {
            std::sort(data.begin(), data.end(), PairCompare());
        }

        return data;
    }
};



template <typename V, Int Stream, template <typename...> class Container, typename... Args>
struct BTFLDataSetBuilder<Container<V, Args...>, Stream, Stream> {

    template <typename SizeT, Int Indexes, typename Rng>
    static auto build(const core::StaticVector<SizeT, Indexes>& level_sizes, Rng&& rng, bool sort)
    {
        Container<V, Args...> data;
        SizeT level_size = Stream == 0 ? level_sizes[Stream] : rng(level_sizes[Stream]);

        for (SizeT c = 0; c < level_size; c++)
        {
            data.emplace_back(fromNumber(c, TypeTag<V>()));
        }

        return data;
    }
};


struct CheckStatus {
	Int entries_;
	bool finished_;
	CheckStatus(Int entries, bool finished): entries_(entries), finished_(finished) {}

	Int entries() 	const {return entries_;}
	bool finished() const {return finished_;}
};



//template <typename BTFLData, Int DataStreams, Int StartLevel = 0, typename IOBufferT = DefaultIOBuffer> class BTFLDataIOBufferCheckerHelper;
//
//
//template <Int DataStreams, Int StartLevel, typename IOBufferT, typename K, typename V, template <typename...> class Container, typename... Args>
//class BTFLDataIOBufferCheckerHelper<Container<std::tuple<K, V>, Args...>, DataStreams, StartLevel, IOBufferT> {
//public:
//	using BTFLDataT = Container<std::tuple<K, V>, Args...>;
//
//protected:
//	using NextBTFLIOBufferChecker  = BTFLDataIOBufferCheckerHelper<V, DataStreams, StartLevel + 1, IOBufferT>;
//	using DataIterator             = typename BTFLDataT::const_iterator;
//
//	bool value_finished_ = true;
//
//	DataIterator start_;
//	DataIterator end_;
//
//	NextBTFLIOBufferChecker next_checker_;
//
//	bool finish_children_ = false;
//
//public:
//	BTFLDataIOBufferCheckerHelper(const DataIterator& start, const DataIterator& end):
//		start_(start),
//		end_(end)
//{}
//
//	BTFLDataIOBufferCheckerHelper(const BTFLDataT& data):
//		start_(data.begin()),
//		end_(data.end())
//	{}
//
//	BTFLDataIOBufferCheckerHelper() {}
//
//	CheckStatus process(IOBufferT& buffer, Int entries)
//	{
//		Int entry = 0;
//
//		if (finish_children_)
//		{
//			auto result = next_checker_.process(buffer, entries);
//			if (result.finished())
//			{
//				finish_children_ = false;
//				entry += result.entries();
//			}
//			else {
//				return result;
//			}
//		}
//
//		while (start_ != end_ && entry < entries)
//		{
//			auto run = buffer.template getSymbolsRun<DataStreams>();
//			entry++;
//
//			if (run.symbol() == StartLevel)
//			{
//				if (run.length() > 0)
//				{
//					if (entry + run.length() <= (UBigInt)entries)
//					{
//							UBigInt s;
//							for (s = 0; s < run.length() - 1 && start_ != end_; s++)
//							{
//								auto key = IOBufferAdapter<K>::get(buffer);
//
//								if (key != std::get<0>(*start_))
//								{
//									throw Exception(MA_SRC, SBuf() << "Key check failed. Expected: " << std::get<0>(*start_) << ", actual: " << key << " for " << run.symbol());
//								}
//
//								if (std::get<1>(*start_).size() != 0)
//								{
//									throw Exception(MA_SRC, SBuf() << "Non-zero value for : " << run.symbol());
//								}
//
//								start_++;
//								entry++;
//							}
//
//							if (s < run.length() - 1) {
//								throw Exception(MA_SRC, SBuf() << "Premature end of data to compare for : " << run.symbol());
//							}
//
//							if (start_ != end_)
//							{
//								auto key = IOBufferAdapter<K>::get(buffer);
//								if (key != std::get<0>(*start_))
//								{
//									throw Exception(MA_SRC, SBuf() << "Key check failed. Expected: " << std::get<0>(*start_) << ", actual: " << key << " for " << run.symbol());
//								}
//
//								entry++;
//
//								next_checker_ = NextBTFLIOBufferChecker(std::get<1>(*start_));
//								auto status = next_checker_.process(buffer, entries - entry);
//								entry += status.entries();
//								start_++;
//
//								if (!status.finished())
//								{
//									finish_children_ = true;
//									return CheckStatus(entry, false);
//								}
//							}
//							else {
//								throw Exception(MA_SRC, SBuf() << "Premature end data to compare for : " << run.symbol());
//							}
//					}
//					else {
//						throw Exception(MA_SRC, SBuf() << "Symbol run is too long for this buffer: " << run.length());
//					}
//				}
//				else {
//					throw Exception(MA_SRC, SBuf() << "Zero symbols run length for: " << run.symbol());
//				}
//			}
//			else {
//				throw Exception(MA_SRC, SBuf() << "Unexpected symbol: " << run.symbol() <<", expected: " << StartLevel);
//			}
//		}
//
//		return CheckStatus(entry, start_ != end_);
//	}
//};
//
//
//
//
//
//template <Int DataStreams, Int StartLevel, typename IOBufferT, typename V, template <typename...> class Container, typename... Args>
//class BTFLDataIOBufferCheckerHelper<Container<V, Args...>, DataStreams, StartLevel, IOBufferT> {
//public:
//	using BTFLDataT = Container<V, Args...>;
//
//protected:
//	using DataIterator               = typename BTFLDataT::const_iterator;
//
//	DataIterator start_;
//	DataIterator end_;
//
//public:
//	BTFLDataIOBufferCheckerHelper(const DataIterator& start, const DataIterator& end):
//		start_(start),
//		end_(end)
//	{}
//
//	BTFLDataIOBufferCheckerHelper(const BTFLDataT& data):
//		start_(data.begin()),
//		end_(data.end())
//	{}
//
//	BTFLDataIOBufferCheckerHelper() {}
//
//
//	CheckStatus process(IOBufferT& buffer, Int entries)
//	{
//		Int entry = 0;
//		while (start_ != end_ && entry < entries)
//		{
//			auto run = buffer.template getSymbolsRun<DataStreams>();
//
//			if (run.symbol() == StartLevel)
//			{
//				entry++;
//				if (entry + run.length() <= (UBigInt)entries)
//				{
//					if (run.length() > 0)
//					{
//						UBigInt s;
//						for (s = 0; s < run.length() && start_ != end_; s++)
//						{
//							auto value = IOBufferAdapter<V>::get(buffer);
//							if (value == *start_)
//							{
//								start_++;
//								entry++;
//							}
//							else {
//								throw Exception(MA_SRC, SBuf() << "Value check failed. Expected: " << *start_ << ", actual: " << value << " for " << run.symbol());
//							}
//						}
//
//						if (s < run.length() - 1) {
//							throw Exception(MA_SRC, SBuf() << "Premature end of data to compare for : " << run.symbol());
//						}
//					}
//					else {
//						throw Exception(MA_SRC, SBuf() << "Zero symbols run length for: " << run.symbol());
//					}
//				}
//				else {
//					throw Exception(MA_SRC, SBuf() << "Symbol run is too long for this buffer: " << run.length());
//				}
//			}
//			else {
//				throw Exception(MA_SRC, SBuf() << "Unexpected symbol: " << run.symbol() <<", expected: " << StartLevel);
//			}
//		}
//
//		return CheckStatus(entry, start_ == end_);
//	}
//};
//
//
//
//template <typename BTFLData, Int DataStreams, Int StartLevel = 0, typename IOBufferT = DefaultIOBuffer>
//class BTFLDataCheckerConsumer: public BufferConsumer<IOBufferT> {
//
//	IOBufferT io_buffer_;
//	BTFLDataIOBufferCheckerHelper<BTFLData, DataStreams, StartLevel, IOBufferT> checker_helper_;
//
//	using DataIterator = typename BTFLData::const_iterator;
//
//public:
//
//	BTFLDataCheckerConsumer(const BTFLData& data, size_t capacity = 512):
//		io_buffer_(capacity),
//		checker_helper_(data)
//	{}
//
//	BTFLDataCheckerConsumer(const DataIterator& start, const DataIterator& end, size_t capacity = 65536):
//		io_buffer_(capacity),
//		checker_helper_(start, end)
//	{}
//
//	virtual IOBufferT& buffer() {return io_buffer_;}
//
//	virtual Int process(IOBufferT& buffer, Int entries)
//	{
//		auto state = checker_helper_.process(buffer, entries);
//
//		MEMORIA_V1_ASSERT(state.entries(), ==, entries);
//		return state.entries();
//	}
//};








}
}}
