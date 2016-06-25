
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

#include <algorithm>

namespace memoria {
namespace v1 {
namespace btfl {

namespace {

    template <
                Int KeyStream,
                Int StreamsMax,
                Int Stream,
                typename KeyType,
                typename ValueType,
                typename ColumnType,
                template <Int, Int> class ContainerFactory
            >
    struct DataTypeBuilder {
        using Type = typename ContainerFactory<KeyStream, Stream>::template Type<
            std::tuple<
                IfThenElse<Stream == KeyStream, KeyType, ColumnType>,
                typename DataTypeBuilder<
                    KeyStream,
                    StreamsMax,
                    Stream - 1,
                    KeyType, ValueType, ColumnType,
                              ContainerFactory
                >::Type
            >
        >;
    };


    template <Int KeyStream, Int StreamsMax, typename KeyType, typename ValueType, typename ColumnType, template <Int, Int> class ContainerFactory>
    struct DataTypeBuilder<KeyStream, StreamsMax, 0, KeyType, ValueType, ColumnType, ContainerFactory> {
        using Type = typename ContainerFactory<KeyStream, 0>::template Type<ValueType>;
    };



    template <Int KeyStream, Int CurrentStream> struct ContainerFactory {
        template <typename V>
        using Type = std::vector<V>;
    };
}


template <typename DataSetType> struct BTFLDataStreamsCounter;

template <typename K, typename V, typename... Args, template <typename...> class Container>
struct BTFLDataStreamsCounter<Container<std::tuple<K, V>, Args...>> {
    static constexpr Int Value = 1 + BTFLDataStreamsCounter<V>::Value;
};

template <typename... V, template <typename...> class Container>
struct BTFLDataStreamsCounter<Container<V...>> {
    static constexpr Int Value = 1;
};






template <
    Int DataStreams,
    Int Level = DataStreams - 1,
    typename KeyType    = BigInt,
    typename ValueType  = UByte,
    typename ColumnType = BigInt,
    template <Int, Int> class ContainerTypeFactory = ContainerFactory
>
using BTFLData = typename DataTypeBuilder<Level, DataStreams - 1, DataStreams - 1, KeyType, ValueType, ColumnType, ContainerTypeFactory>::Type;












}
}}
