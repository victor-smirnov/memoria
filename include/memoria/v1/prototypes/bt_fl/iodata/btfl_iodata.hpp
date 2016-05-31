
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>
#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_input.hpp>
#include <memoria/v1/prototypes/bt_fl/iodata/btfl_iodata_decl.hpp>
#include <memoria/v1/prototypes/bt_fl/iodata/btfl_iodata_producer.hpp>
#include <memoria/v1/prototypes/bt_fl/iodata/btfl_iodata_reader.hpp>

#include <algorithm>

namespace memoria {
namespace v1 {
namespace btfl {



template <typename BTFLData> class BTFLDataComputeLengthHelper;


template <typename K, typename V, template <typename...> class Container, typename... Args>
class BTFLDataComputeLengthHelper<Container<std::tuple<K, V>, Args...>> {
public:
    using BTFLDataT = Container<std::tuple<K, V>, Args...>;

protected:
    using NextBTFLDataComputeLengthHelper = BTFLDataComputeLengthHelper<V>;
    using DataIterator                          = typename BTFLDataT::const_iterator;

    DataIterator start_;
    DataIterator end_;

    NextBTFLDataComputeLengthHelper next_helper_;

public:
    BTFLDataComputeLengthHelper(const DataIterator& start, const DataIterator& end):
        start_(start),
        end_(end)
    {}

    BTFLDataComputeLengthHelper(const BTFLDataT& data):
        start_(data.begin()),
        end_(data.end())
    {}

    BTFLDataComputeLengthHelper() {}

    size_t compute()
    {
        size_t length = 0;
        while (start_ != end_)
        {
            length++;
            next_helper_ = NextBTFLDataComputeLengthHelper(std::get<1>(*start_));
            length += next_helper_.compute();
            start_++;
        }

        return length;
    }
};




template <typename V, template <typename...> class Container, typename... Args>
class BTFLDataComputeLengthHelper<Container<V, Args...>> {
public:
    using BTFLDataT = Container<V, Args...>;

protected:
    using DataIterator = typename BTFLDataT::const_iterator;


    DataIterator start_;
    DataIterator end_;

public:
    BTFLDataComputeLengthHelper(const DataIterator& start, const DataIterator& end):
        start_(start),
        end_(end)
    {}

    BTFLDataComputeLengthHelper(const BTFLDataT& data):
        start_(data.begin()),
        end_(data.end())
    {}

    BTFLDataComputeLengthHelper() {}

    size_t compute()
    {
        return std::distance(start_, end_);
    }
};



}
}}
