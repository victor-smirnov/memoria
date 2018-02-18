
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_ctr_input_provider_base.hpp>


#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {




template <
    typename CtrT,
    typename IOBuffer
>
class IOBufferCtrInputProvider: public v1::btfl::io::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength> {
public:
    using Base = v1::btfl::io::AbstractCtrInputProvider<CtrT, CtrT::Types::Streams, CtrT::Types::LeafDataLength>;


    using typename Base::DataPositions;

protected:

    using typename Base::ForAllDataStreams;

    using typename Base::DataStreamBuffers;
    using typename Base::StructureStreamBuffer;

    using Base::DataStreams;


    using Base::reset_buffer;
    using Base::finish_buffer;

    using Base::start_;
    using Base::size_;
    using Base::data_buffers_;
    using Base::structure_buffer_;
    using Base::symbols_;
    using Base::finished_;
    using Base::total_symbols_;
    using Base::totals_;




    int32_t stream_run_remainder_ = 0;

    bt::BufferProducer<IOBuffer>* iobuffer_producer_ = nullptr;
    IOBuffer* io_buffer_ = nullptr;

    int32_t last_stream_ = -1;

public:
    IOBufferCtrInputProvider(CtrT& ctr, int32_t initial_capacity = 4000):
        Base(ctr, initial_capacity)
    {}

    void init(bt::BufferProducer<IOBuffer>* iobuffer_producer, IOBuffer* io_buffer)
    {
        Base::init();

        stream_run_remainder_  = 0;
        last_stream_           = -1;

        iobuffer_producer_  = iobuffer_producer;
        io_buffer_          = io_buffer;
    }

    void clear()
    {

    }


    struct AppendStreamEntriesFn {
        template <int32_t Idx, typename InputBuffer>
        void process(InputBuffer& input_buffer, int32_t stream, int32_t length, IOBuffer& io_buffer)
        {
            if (Idx == stream)
            {
                input_buffer.append_stream_entries(length, io_buffer);
            }
        }
    };


    void append_data_streams_entries(int32_t stream, int32_t length, IOBuffer& io_buffer)
    {
        ForAllDataStreams::process(data_buffers_, AppendStreamEntriesFn(), stream, length, io_buffer);
    }

    virtual void do_populate_iobuffer()
    {
        reset_buffer();

        DataPositions sizes;
        DataPositions buffer_sums;

        start_.clear();
        size_.clear();

        io_buffer_->rewind();
        int32_t entries = iobuffer_producer_->populate(*io_buffer_);
        io_buffer_->rewind();

        symbols_.prepare();

        if (entries != 0)
        {
            if (entries < 0)
            {
                finished_ = true;
                entries = -entries;
            }

            for (int32_t entry_num = 0; entry_num < entries;)
            {
                int32_t stream;
                int32_t run_length;
                bool premature_eob = false;

                if (stream_run_remainder_ != 0)
                {
                    stream = last_stream_;

                    if (stream_run_remainder_ <= entries)
                    {
                        run_length              = stream_run_remainder_;
                        stream_run_remainder_   = 0;
                    }
                    else {
                        run_length              = entries;
                        stream_run_remainder_   -= entries;
                        premature_eob           = true;
                    }
                }
                else {
                    auto run = io_buffer_->template getSymbolsRun<DataStreams>();

                    stream      = run.symbol();
                    run_length  = run.length();

                    entry_num++;

                    int32_t iobuffer_remainder = entries - entry_num;

                    if (run_length >= iobuffer_remainder)
                    {
                        stream_run_remainder_   = run_length - iobuffer_remainder;
                        run_length              -= stream_run_remainder_;
                        premature_eob           = true;
                    }
                }

                append_data_streams_entries(stream, run_length, *io_buffer_);

                symbols_.append_run(stream, run_length);
                structure_buffer_.append_run(stream, run_length);

                total_symbols_  += run_length;
                size_[stream]   += run_length;

                sizes[stream]       += run_length;
                totals_[stream]     += run_length;

                last_stream_ = stream;

                entry_num += run_length;
            }

            symbols_.reindex();

            finish_buffer();
        }
        else {
            finished_ = true;
        }
    }
};







}}}}
