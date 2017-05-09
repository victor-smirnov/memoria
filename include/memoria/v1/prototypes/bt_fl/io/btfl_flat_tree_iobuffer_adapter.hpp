
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_data_input.hpp>
#include <memoria/v1/prototypes/bt_fl/io/btfl_structure_input.hpp>
#include <memoria/v1/prototypes/bt_fl/io/btfl_rank_dictionary.hpp>
#include <memoria/v1/prototypes/bt_fl/io/btfl_flat_tree_structure_generator.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {

template <int32_t DataStreams, typename IOBufferT>
class FlatTreeIOBufferAdapter: public bt::BufferProducer<IOBufferT> {

public:

    static constexpr int64_t MaxRunLength = IOBufferT::template getMaxSymbolsRunLength<DataStreams>();

    using CtrSizesT = core::StaticVector<int64_t, DataStreams>;

    using IOBuffer = IOBufferT;

private:

    RunDescr state_;
    int32_t processed_ = 0;
    int64_t run_length_ = 0;
    int64_t run_processed_ = 0;
    bool symbol_encoded_ = false;

    CtrSizesT consumed_;

public:
    FlatTreeIOBufferAdapter(){}

    const CtrSizesT& consumed() const {
        return consumed_;
    }

    virtual RunDescr query() = 0;
    virtual int32_t populate_stream(int32_t stream, IOBuffer& buffer, int32_t length) = 0;

    virtual int32_t populate(IOBuffer& io_buffer)
    {
        int32_t entries = 0;

        while (true)
        {
            if (processed_ == state_.length())
            {
                state_          = this->query();
                symbol_encoded_ = false;
                processed_      = 0;
            }

            if (state_.symbol() >= 0)
            {
                auto length = state_.length();

                while (processed_ < length || length == 0)
                {
                    int32_t remainder = length - processed_;

                    if (run_processed_ == run_length_)
                    {
                        run_length_ = remainder > MaxRunLength ? MaxRunLength : remainder;
                        run_processed_  = 0;
                        symbol_encoded_ = false;
                    }

                    int32_t to_encode = run_length_ - run_processed_;

                    if (!symbol_encoded_)
                    {
                        auto pos = io_buffer.pos();
                        if (io_buffer.template putSymbolsRun<DataStreams>(state_.symbol(), to_encode))
                        {
                            symbol_encoded_ = true;
                            entries++;
                        }
                        else {
                            io_buffer.pos(pos);
                            symbol_encoded_ = false;
                            return entries;
                        }
                    }

                    if (to_encode > 0)
                    {
                        int32_t actual = populate_stream(state_.symbol(), io_buffer, to_encode);

                        processed_      += actual;
                        run_processed_  += actual;
                        entries         += actual;

                        consumed_[state_.symbol()] += actual;

                        if (actual < to_encode)
                        {
                            return entries;
                        }
                        else {
                            symbol_encoded_ = false;
                        }
                    }
                }
            }
            else {
                return -entries;
            }
        }

        return entries;
    }
};



}}}}
