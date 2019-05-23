
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/iovector/io_symbol_sequence.hpp>
#include <memoria/v1/core/iovector/io_substream.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memoria/v1/core/types/list/map.hpp>
#include <memoria/v1/core/types/list/typelist.hpp>

#include <vector>
#include <tuple>

namespace memoria {
namespace v1 {
namespace io {

struct IOVector {
    virtual ~IOVector() noexcept {}

    virtual void reset() = 0;
    virtual void reindex() = 0;

    virtual void add_substream(std::unique_ptr<IOSubstream>&& ptr) = 0;
    virtual IOSymbolSequence& symbol_sequence() = 0;

    virtual const IOSymbolSequence& symbol_sequence() const = 0;

    virtual size_t substreams() const = 0;

    virtual IOSubstream& substream(size_t num) = 0;
    virtual const IOSubstream& substream(size_t num) const = 0;

    virtual int32_t streams() const = 0;
    virtual int32_t stream_size(int32_t stream) const = 0;
};


template <int32_t SubstreamsNum>
class DefaultIOVector: public IOVector {

    std::unique_ptr<IOSymbolSequence> symbol_sequence_;

    struct CleanupFn {
        template <typename Value>
        void operator()(Value& val)
        {}
    };

    core::StaticArray<std::unique_ptr<IOSubstream>, SubstreamsNum, CleanupFn> substreams_;

    std::vector<int32_t> schema_{};

public:
    DefaultIOVector(int32_t symbols):
        symbol_sequence_(make_packed_rle_symbol_sequence(symbols))
    {}

    void reset()
    {
        symbol_sequence_->reset();

        for (int32_t c = 0; c < SubstreamsNum; c++) {
            substreams_[c]->reset();
        }
    }

    void add_substream(std::unique_ptr<IOSubstream>&& ptr)
    {
        substreams_.append(std::move(ptr));
    }

    void add_stream_schema(int32_t substreams)
    {
        schema_.push_back(substreams);
    }

    IOSymbolSequence& symbol_sequence() {
        return *symbol_sequence_.get();
    }

    const IOSymbolSequence& symbol_sequence() const {
        return *symbol_sequence_.get();
    }

    size_t substreams() const {
        return substreams_.size();
    }

    IOSubstream& substream(size_t num) {
        return *substreams_[num].get();
    }

    const IOSubstream& substream(size_t num) const {
        return *substreams_[num].get();
    }

    int32_t streams() const {
        return schema_.size();
    }

    int32_t stream_size(int32_t stream) const
    {
        return schema_[stream];
    }

    virtual void reindex()
    {
        symbol_sequence_->reindex();
        for (int32_t c = 0; c < SubstreamsNum; c++) {
            substreams_[c]->reindex();
        }
    }
};

namespace _ {

    template <typename Tuple, int32_t Idx = 0, int32_t Max = std::tuple_size<Tuple>::value>
    struct IOVectorSubstreamsInitializer {

        template <typename StreamsArray>
        static void process(Tuple& tuple, StreamsArray& streams)
        {
            streams[Idx] = & std::get<Idx>(tuple);
            IOVectorSubstreamsInitializer<Tuple, Idx + 1, Max>::process(tuple, streams);
        }
    };

    template <typename Tuple, int32_t Idx>
    struct IOVectorSubstreamsInitializer<Tuple, Idx, Idx> {
        template <typename StreamsArray>
        static void process(Tuple& tuple, StreamsArray& streams){}
    };


    template <typename List> struct ValueListToArray;

    template <int32_t Head, int32_t... Tail>
    struct ValueListToArray<IntList<Head, Tail...>> {
        template <typename Array>
        static void append_to(Array& array)
        {
            array.append(Head);
            ValueListToArray<IntList<Tail...>>::append_to(array);
        }
    };

    template <>
    struct ValueListToArray<IntList<>> {
        template <typename Array>
        static void append_to(Array& array) {}
    };
}

template <typename Types, template <typename> class IOVectorMapperTF>
class StaticIOVector: public IOVector {

    using InputVectorsList  = MapTL2<typename Types::PackedStructsList, IOVectorMapperTF>;
    using IVTuple           = AsTuple<InputVectorsList>;
    using StreamsSchema     = typename Types::StreamsSchema;

    typename Types::SymbolSequence symbol_sequence_;

    IVTuple streams_tuple_;

    struct CleanupFn {
        template <typename Value>
        void operator()(Value& val)
        {}
    };

    static constexpr int32_t SubstreamsNum = std::tuple_size<IVTuple>::value;

    core::StaticArray<IOSubstream*, SubstreamsNum, CleanupFn> substreams_;

    core::StaticArray<int32_t, ListSize<StreamsSchema>> schema_{};

public:
    StaticIOVector():
        symbol_sequence_(),
        substreams_(SubstreamsNum)
    {
        _::IOVectorSubstreamsInitializer<IVTuple>::process(streams_tuple_, substreams_);
        _::ValueListToArray<StreamsSchema>::append_to(schema_);
    }

    void reset()
    {
        symbol_sequence_.reset();

        for (int32_t c = 0; c < SubstreamsNum; c++) {
            substreams_[c]->reset();
        }
    }

    void add_substream(std::unique_ptr<IOSubstream>&& ptr)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    IOSymbolSequence& symbol_sequence() {
        return symbol_sequence_;
    }

    const IOSymbolSequence& symbol_sequence() const {
        return symbol_sequence_;
    }

    size_t substreams() const {
        return substreams_.size();
    }

    IOSubstream& substream(size_t num) {
        return *substreams_[num];
    }

    const IOSubstream& substream(size_t num) const {
        return *substreams_[num];
    }

    int32_t streams() const {
        return schema_.size();
    }

    int32_t stream_size(int32_t stream) const
    {
        return schema_[stream];
    }

    virtual void reindex()
    {
        symbol_sequence_.reindex();
        for (int32_t c = 0; c < SubstreamsNum; c++) {
            substreams_[c]->reindex();
        }
    }
};



struct IOVectorProducer {
    virtual ~IOVectorProducer() noexcept {}
    virtual bool populate(IOVector& io_vector) = 0;
};

struct IOVectorConsumer {
    virtual ~IOVectorConsumer() noexcept {}
    virtual void consume(IOVector& io_vector) = 0;
};




template <
        typename SubstreamTypeTag_,
        template <typename> class Codec_
>
struct SSTraits
{
    using SubstreamTypeTag = SubstreamTypeTag_;

    template <typename T> using Codec = Codec_<T>;
};



}}}
