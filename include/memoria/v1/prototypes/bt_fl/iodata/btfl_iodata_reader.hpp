
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

#include <algorithm>

namespace memoria {
namespace v1 {
namespace btfl {


template <typename BTFLData, Int DataStreams, Int StartLevel = 0, typename IOBufferT = DefaultIOBuffer> class BTFLDataIOBufferReaderHelper;


template <Int DataStreams, Int StartLevel, typename IOBufferT, typename K, typename V, template <typename...> class Container, typename... Args>
class BTFLDataIOBufferReaderHelper<Container<std::tuple<K, V>, Args...>, DataStreams, StartLevel, IOBufferT> {
public:
    using BTFLDataT = Container<std::tuple<K, V>, Args...>;

protected:
    using NextBTFLIOBufferReader   = BTFLDataIOBufferReaderHelper<V, DataStreams, StartLevel + 1, IOBufferT>;
    using DataIterator             = typename BTFLDataT::const_iterator;

    BTFLDataT* data_;

    NextBTFLIOBufferReader next_reader_;


public:
    BTFLDataIOBufferReaderHelper(BTFLDataT& data):
        data_(&data)
    {}

    BTFLDataIOBufferReaderHelper() {}

    void process(Int stream, IOBufferT& buffer, Int entries)
    {
        if (stream == StartLevel)
        {
            MEMORIA_V1_ASSERT_NOT_NULL(data_);
            for (Int e = 0; e < entries; e++)
            {
                auto key = IOBufferAdapter<K>::get(buffer);
                data_->emplace_back(std::tuple<K, V>(key, V()));

                next_reader_ = NextBTFLIOBufferReader(std::get<1>(data_->back()));
            }
        }
        else {
            next_reader_.process(stream, buffer, entries);
        }
    }
};





template <Int DataStreams, Int StartLevel, typename IOBufferT, typename V, template <typename...> class Container, typename... Args>
class BTFLDataIOBufferReaderHelper<Container<V, Args...>, DataStreams, StartLevel, IOBufferT> {
public:
    using BTFLDataT = Container<V, Args...>;

protected:
    using DataIterator               = typename BTFLDataT::const_iterator;

    BTFLDataT* data_ = nullptr;

public:

    BTFLDataIOBufferReaderHelper(BTFLDataT& data):
        data_(&data)
    {}

    BTFLDataIOBufferReaderHelper() {}


    void process(Int stream, IOBufferT& buffer, Int entries)
    {
        if (stream == StartLevel)
        {
            MEMORIA_V1_ASSERT_NOT_NULL(data_);
            for (Int e = 0; e < entries; e++)
            {
                auto value = IOBufferAdapter<V>::get(buffer);
                data_->emplace_back(value);
            }
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream << " max: " << DataStreams);
        }
    }
};



template <typename BTFLData, Int DataStreams, Int StartLevel = 0, typename IOBufferT = DefaultIOBuffer>
class BTFLDataReader: public BufferConsumer<IOBufferT> {

    using Helper = BTFLDataIOBufferReaderHelper<BTFLData, DataStreams, StartLevel, IOBufferT>;

    IOBufferT io_buffer_;
    Helper reader_helper_;

public:

    BTFLDataReader(size_t capacity = 65536):
        io_buffer_(capacity),
        reader_helper_()
    {}

    void init(BTFLData& data)
    {
        io_buffer_.rewind();
        reader_helper_ = Helper(data);
    }

    void clear() {}


    virtual IOBufferT& buffer() {return io_buffer_;}

    virtual Int process(IOBufferT& buffer, Int entries)
    {
    	Int e;
    	for (e = 0; e < entries;)
        {
            auto run = buffer.template getSymbolsRun<DataStreams>();
            reader_helper_.process(run.symbol(), buffer, run.length());

            e += run.length() + 1;
        }

        return entries;
    }
};


}
}}
