
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
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>
#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/retired/prototypes/bt_fl/io/btfl_input.hpp>

namespace memoria {
namespace v1 {
namespace btfl {
namespace iobuf {



template <typename CtrT, typename IOBufferT = DefaultIOBuffer>
class DeterministicAdapterBase: public btfl::io::FlatTreeIOBufferAdapter<CtrT::Types::Streams, IOBufferT> {

protected:

    static constexpr int32_t Streams = CtrT::Types::Streams;

    using Base      = btfl::io::FlatTreeIOBufferAdapter<Streams, IOBufferT>;
    using MyType    = DeterministicAdapterBase<CtrT, IOBufferT>;

    using IOBuffer = IOBufferT;

    using CtrSizesT = typename CtrT::Types::CtrSizesT;

    CtrSizesT structure_;
    IOBufferT io_buffer_;

    int32_t level_ = 0;

    struct StructureGenerator: public btfl::io::FlatTreeStructureGeneratorBase<StructureGenerator, Streams> {

        MyType* adapter_;

        StructureGenerator(MyType* adapter, int32_t level):
            btfl::io::FlatTreeStructureGeneratorBase<StructureGenerator, Streams>(level),
            adapter_(adapter)
        {}


        auto prepare(const bt::StreamTag<0>&)
        {
            return adapter_->structure()[0];
        }

        template <int32_t Idx, typename Pos>
        auto prepare(const bt::StreamTag<Idx>&, const Pos& pos)
        {
            return adapter_->structure()[Idx];
        }
    };


    StructureGenerator structure_generator_;

public:

    DeterministicAdapterBase(const CtrSizesT& structure, int32_t level = 0, size_t iobuffer_size = 65536):
        structure_(structure),
        io_buffer_(iobuffer_size),
        structure_generator_(this, level)
    {
        structure_generator_.init();
    }

    const CtrSizesT& structure() {
        return structure_;
    }

    StructureGenerator& structure_generator() {
        return structure_generator_;
    }

    const StructureGenerator& structure_generator() const {
        return structure_generator_;
    }


    virtual IOBufferT& buffer() {return io_buffer_;}

    virtual btfl::io::RunDescr query()
    {
        return structure_generator_.query();
    }

    virtual int32_t populate_stream(int32_t stream, IOBufferT& buffer, int32_t length) = 0;
};




template <typename CtrT, typename RngT, typename IOBufferT = DefaultIOBuffer>
class RandomAdapterBase: public btfl::io::FlatTreeIOBufferAdapter<CtrT::Types::Streams, IOBufferT> {
public:
    using Rng = RngT;
protected:

    using IOBuffer = IOBufferT;

    static constexpr int32_t Streams = CtrT::Types::Streams;

    using Base      = btfl::io::FlatTreeIOBufferAdapter<Streams, IOBufferT>;
    using MyType    = RandomAdapterBase<CtrT, RngT, IOBufferT>;

    using CtrSizesT = typename CtrT::Types::CtrSizesT;

    CtrSizesT structure_;
    IOBufferT io_buffer_;

    int32_t level_ = 0;

    struct StructureGenerator: public btfl::io::FlatTreeStructureGeneratorBase<StructureGenerator, Streams> {

        MyType* adapter_;

        StructureGenerator(MyType* adapter, int32_t level):
            btfl::io::FlatTreeStructureGeneratorBase<StructureGenerator, Streams>(level),
            adapter_(adapter)
        {}


        auto prepare(const bt::StreamTag<0>&)
        {
            return adapter_->structure()[0];
        }

        template <int32_t Idx, typename Pos>
        auto prepare(const bt::StreamTag<Idx>&, const Pos& pos)
        {
            auto limit = adapter_->structure()[Idx];
            int64_t v = adapter_->rng()();

            return 1 + v % (2 * limit - 1);
        }
    };


    StructureGenerator structure_generator_;

    RngT rng_;

public:

    RandomAdapterBase(const CtrSizesT& structure, const RngT& rng, int32_t level = 0, size_t iobuffer_size = 65536):
        structure_(structure),
        io_buffer_(iobuffer_size),
        structure_generator_(this, level)
    {
        structure_generator_.init();
    }

    const CtrSizesT& structure() {
        return structure_;
    }

    RngT& rng() {return rng_;}
    const RngT& rng() const {return rng_;}

    virtual IOBufferT& buffer() {return io_buffer_;}

    virtual btfl::io::RunDescr query()
    {
        return structure_generator_.query();
    }

    StructureGenerator& structure_generator() {
        return structure_generator_;
    }

    const StructureGenerator& structure_generator() const {
        return structure_generator_;
    }

    virtual int32_t populate_stream(int32_t stream, IOBufferT& buffer, int32_t length) = 0;
};






}

}
}}
