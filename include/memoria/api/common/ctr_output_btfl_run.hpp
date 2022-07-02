
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

#include <memoria/api/common/ctr_api.hpp>

#include <memoria/core/bignum/int64_codec.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/static_array.hpp>

namespace memoria {

/*

template <typename MyType>
class BTFLRunParserBase {

    bool process_prefix_{};

    size_t prefix_size_{};
    size_t suffix_size_{};

    io::SymbolsBuffer buffer_;

    int32_t keys_sym_{0};
    int32_t data_sym_{1};

    int32_t start_idx_{};

    int32_t last_idx_{};
    bool finished_{};
    bool empty_{};

public:
    BTFLRunParserBase(uint64_t start_idx, int32_t alphabet_size):
        buffer_(alphabet_size),
        start_idx_(start_idx)
    {
        self().on_parse_start(start_idx_);
    }

    void on_parse_start(uint64_t start_idx) {}
    void on_next_block() {}

    MyType& self() {return *static_cast<MyType*>(this);}
    const MyType& self() const {return *static_cast<const MyType*>(this);}

    void parse(const io::IOSSRLEBufferBase& sequence)
    {
        buffer_.clear();

        uint64_t last_idx = sequence.populate_buffer_while_ge(buffer_, start_idx_, data_sym_);
        finished_ = last_idx < (size_t)sequence.size();

        last_idx_ = last_idx;

        if (process_prefix_) {
            self().on_next_block();
        }

        process_prefix_ = true;
        start_idx_ = 0;
    }

    int32_t start_idx() const {
        return start_idx_;
    }

    int32_t last_idx() const {
        return last_idx_;
    }

    bool is_finished() const {
        return finished_;
    }

    bool is_empty() const {
        return buffer_.size() == 0;
    }

    size_t run_size() const {
        return buffer_[0].length;
    }

    const io::SymbolsBuffer& buffer() const {
        return buffer_;
    }

    bool process_prefix() const {
        return process_prefix_;
    }
};

template <int32_t Streams>
class DefaultBTFLRunParser: public BTFLRunParserBase<DefaultBTFLRunParser<Streams>> {
    using Base = BTFLRunParserBase<DefaultBTFLRunParser<Streams>>;
public:

    DefaultBTFLRunParser(uint64_t start):
        Base(start, Streams)
    {}
};

*/

}
