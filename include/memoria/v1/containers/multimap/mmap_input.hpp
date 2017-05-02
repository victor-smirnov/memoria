
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

#include <memoria/v1/containers/multimap/mmap_tools.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace v1 {
namespace mmap {

template <typename K, typename V>
using MapData = std::vector<std::pair<K, std::vector<V>>>;


template <typename Key, typename Value, typename IOBufferT = DefaultIOBuffer>
class MultimapIOBufferProducer: public btfl::io::FlatTreeIOBufferAdapter<2, IOBufferT> {

    using Base   = btfl::io::FlatTreeIOBufferAdapter<2, IOBufferT>;
    using MyType = MultimapIOBufferProducer<Key, Value, IOBufferT>;

    using typename Base::IOBuffer;

    using Data      = MapData<Key, Value>;
    using Positions = core::StaticVector<int32_t, 2>;


    const Data& data_;

    IOBuffer io_buffer_;
    Positions positions_;
    int32_t level_ = 0;

    struct StructureAdapter: public btfl::io::FlatTreeStructureGeneratorBase<StructureAdapter, 2> {
        MyType* adapter_;
        StructureAdapter(MyType* adapter):
            adapter_(adapter)
        {}


        auto prepare(const StreamTag<0>&)
        {
            return adapter_->data().size();
        }

        template <int32_t Idx, typename Pos>
        auto prepare(const StreamTag<Idx>&, const Pos& pos)
        {
            return adapter_->data()[pos[Idx - 1]].second.size();
        }
    };


    StructureAdapter structure_generator_;

public:


    MultimapIOBufferProducer(const Data& data, size_t iobuffer_size = 65536):
        data_(data),
        io_buffer_(iobuffer_size),
        structure_generator_(this)
    {
        structure_generator_.init();
    }

    const Data& data() {return data_;}

    virtual IOBuffer& buffer() {return io_buffer_;}

    virtual btfl::io::RunDescr query()
    {
        return structure_generator_.query();
    }

    virtual int32_t populate_stream(int32_t stream, IOBuffer& buffer, int32_t length)
    {
        if (stream == 1)
        {
            auto& idx    = structure_generator_.counts()[1];
            auto key_idx = structure_generator_.counts()[0];

            const auto& data = data_[key_idx - 1].second;

            int32_t c;
            for (c = 0; c < length; c++, idx++)
            {
                auto pos = buffer.pos();
                if (!IOBufferAdapter<Value>::put(buffer, data[idx]))
                {
                    buffer.pos(pos);
                    return c;
                }
            }

            return c;
        }
        else {
            auto& idx = structure_generator_.counts()[0];

            int32_t c;
            for (c = 0; c < length; c++, idx++)
            {
                auto pos = buffer.pos();
                if (!IOBufferAdapter<Key>::put(buffer, data_[idx].first))
                {
                    buffer.pos(pos);
                    return c;
                }
            }

            return c;
        }
    }

};





}
}}
