
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

#include <memoria/v1/tests/assertions.hpp>

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




template <int32_t DataStreams, int32_t Level = DataStreams - 1, typename KeyType = int64_t, typename ValueType = uint8_t, typename ColumnType = int64_t>
using BTFLData = btfl::BTFLData<DataStreams, Level, KeyType, ValueType, ColumnType>;



template <typename N, typename T>
T fromNumber(N&& value, const TypeTag<T>&) {
    return value;
}



template <typename DataSetType, int32_t Stream = 0, int32_t DataStreamsMax = btfl::BTFLDataStreamsCounter<DataSetType>::Value - 1>
struct BTFLDataSetBuilder;

template <typename K, typename V, int32_t Stream, int32_t DataStreamsMax, template <typename...> class Container, typename... Args>
struct BTFLDataSetBuilder<Container<std::tuple<K, V>, Args...>, Stream, DataStreamsMax> {

    template <typename SizeT, int32_t Indexes, typename Rng>
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



template <typename V, int32_t Stream, template <typename...> class Container, typename... Args>
struct BTFLDataSetBuilder<Container<V, Args...>, Stream, Stream> {

    template <typename SizeT, int32_t Indexes, typename Rng>
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






template <typename BTFLData, int32_t DataStreams, int32_t StartLevel = 0> class BTFLDataChecker;


template <int32_t DataStreams, int32_t StartLevel, typename K, typename V, template <typename...> class Container, typename... Args>
class BTFLDataChecker<Container<std::tuple<K, V>, Args...>, DataStreams, StartLevel> {
public:
    using BTFLDataT = Container<std::tuple<K, V>, Args...>;

protected:
    using NextBTFLDataChecker      = BTFLDataChecker<V, DataStreams, StartLevel + 1>;
    using DataIterator             = typename BTFLDataT::const_iterator;

    bool value_finished_ = true;

    DataIterator start_;
    DataIterator end_;

    NextBTFLDataChecker next_checker_;

public:
    BTFLDataChecker(const DataIterator& start, const DataIterator& end):
        start_(start),
        end_(end)
    {}

    BTFLDataChecker(const BTFLDataT& data):
        start_(data.begin()),
        end_(data.end())
    {}

    BTFLDataChecker() {}

    std::string lvl() const {
    	return std::string(StartLevel, ' ');
    }

    template <typename CtrIterator>
    void check(CtrIterator&& iter)
    {
    	for (size_t child_idx = 0; start_ != end_; child_idx++)
    	{
            auto key = iter.template key<StartLevel>();
            //auto key = iter.key0();
            tests::assert_equals(key, std::get<0>(*start_));

            auto children = iter.count_children();

            tests::assert_equals((size_t)children, std::get<1>(*start_).size());

    		if (children > 0)
    		{
    			next_checker_ = NextBTFLDataChecker(std::get<1>(*start_));

                iter.to_child(child_idx);
    			next_checker_.check(iter);
                iter.to_parent(StartLevel);
    		}

    		start_++;
            iter.select_ge_fw(1, StartLevel);
    	}
    }
};




template <int32_t DataStreams, int32_t StartLevel, typename V, template <typename...> class Container, typename... Args>
class BTFLDataChecker<Container<V, Args...>, DataStreams, StartLevel> {
public:
    using BTFLDataT = Container<V, Args...>;

protected:
    using DataIterator = typename BTFLDataT::const_iterator;

    DataIterator start_;
    DataIterator end_;

public:
    BTFLDataChecker(const DataIterator& start, const DataIterator& end):
        start_(start),
        end_(end)
    {}

    BTFLDataChecker(const BTFLDataT& data):
        start_(data.begin()),
        end_(data.end())
    {
    }

    BTFLDataChecker() {}


    template <typename CtrIterator>
    void check(CtrIterator&& iter)
    {
    	while (start_ != end_)
    	{
            auto value = iter.template key<StartLevel>();
            tests::assert_equals(value, *start_);

    		start_++;
            iter.next();
    	}
    }

};








}
}}
