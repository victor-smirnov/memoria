
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

#include <memoria/v1/api/common/ctr_api_btfl.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/iovector/io_symbol_sequence.hpp>

#include <memory>
#include <tuple>
#include <exception>

namespace memoria {
namespace v1 {

template <typename MyType>
class MultimapSequenceParserBase {

    static_assert(
        std::is_base_of<MultimapSequenceParserBase<MyType>, MyType>::value,
        "Invalid MyType class"
    );

    bool process_prefix_{};

    uint64_t prefix_offset_{};
    uint64_t prefix_size_{};

    uint64_t suffix_offset_{};
    uint64_t suffix_size_{};

    io::IOBufferBase<int32_t> values_offsets_;
    io::SymbolsBuffer buffer_;

    uint64_t value_starts_[2]{};

public:
    MultimapSequenceParserBase() {}

    MyType& self() {return *static_cast<MyType*>(this);}
    const MyType& self() const {return *static_cast<const MyType*>(this);}

    void parse(io::IOSymbolSequence& sequence, uint64_t start)
    {
        buffer_.reset();
        values_offsets_.reset();

        sequence.rank_to(start, value_starts_);
        sequence.populate_buffer(buffer_, start);

        if (MMA1_LIKELY(buffer_.size() > 0))
        {
            size_t run_start{};

            if (start == 0)
            {
                if (MMA1_LIKELY(process_prefix_))
                {
                    if (MMA1_LIKELY(buffer_[0].symbol == 1)) {
                        prefix_size_ = buffer_[0].length;
                        run_start = 1;
                    }
                    else {
                        prefix_size_ = 0;
                    }
                }
                else
                {

                }
            }
            else {

            }

            size_t run_end = buffer_.size();

            if (MMA1_LIKELY(buffer_.head().symbol == 1))
            {
                process_prefix_ = true;
                suffix_size_    = buffer_.head().length;

                run_end -= 1;
            }

            for (size_t c = run_start; c < run_end; c++)
            {

            }
        }
        else {
            // ...
        }
    }


};

class DefaultMultimapSequenceParser: public MultimapSequenceParserBase<DefaultMultimapSequenceParser> {
public:
};

}}
