
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
#include <memoria/core/iovector/io_symbol_sequence.hpp>


namespace memoria {

template <typename IOBuffer>
struct IBTFLPopulateWalker: Referenceable {
    virtual int32_t populate(IOBuffer* buffer) = 0;
};

namespace _ {

template <typename CtrT, typename WalkerT, typename IOBuffer>
class PopulateWalkerHanlder: public IBTFLPopulateWalker<IOBuffer> {
    CtrT* self_;
    WalkerT walker_;

public:
    PopulateWalkerHanlder(CtrT* self, WalkerT&& walker):
        self_(self), walker_(std::move(walker))
    {}

    virtual int32_t populate(IOBuffer* buffer) {
        return self_->bulkio_populate(*walker_.get(), buffer);
    }
};

}

template <typename IOBuffer, typename CtrT, typename WalkerT>
auto make_btfl_populate_walker_handler(CtrT& ctr, WalkerT&& walker)
{
    return std::unique_ptr<IBTFLPopulateWalker<IOBuffer>>(new _::PopulateWalkerHanlder<CtrT, WalkerT, IOBuffer>{
        &ctr, std::move(walker)
    });
}


template <typename MyType>
class BTFLSequenceParserBase {

    bool process_prefix_{};

    size_t prefix_size_{};
    size_t suffix_size_{};

    io::SymbolsBuffer buffer_;

    int32_t keys_sym_{0};
    int32_t data_sym_{1};

    int32_t start_idx_{};

public:
    BTFLSequenceParserBase(uint64_t start_idx, int32_t alphabet_size):
        buffer_(alphabet_size),
        start_idx_(start_idx)
    {
        self().on_parse_start(start_idx_);
    }

    void on_parse_start(uint64_t start_idx) {}
    void on_next_block() {}

    void process_prefix(const io::SymbolsBuffer& buffer) {}

    void process_body(const io::SymbolsBuffer& buffer, size_t body_start_idx, size_t body_end_idx) {}
    void process_suffix(const io::SymbolsBuffer& buffer, size_t suffix_start_idx) {}


    MyType& self() {return *static_cast<MyType*>(this);}
    const MyType& self() const {return *static_cast<const MyType*>(this);}

    void parse(const io::IOSymbolSequence& sequence)
    {
        buffer_.clear();

        sequence.populate_buffer(buffer_, start_idx_);

        if (process_prefix_) {
            self().on_next_block();
        }

        size_t buffer_size = buffer_.size();

        if (MMA1_LIKELY(buffer_size > 2))
        {
            int32_t first_symbol = buffer_[0].symbol;
            if (MMA1_LIKELY(process_prefix_))
            {
                if (MMA1_LIKELY(first_symbol == data_sym_ && process_prefix_))
                {
                    prefix_size_ = 1;
                    self().process_prefix(buffer_);
                }
                else {
                    prefix_size_ = 0;
                }
            }
            else if (first_symbol == data_sym_) {
                MMA1_THROW(RuntimeException()) << WhatCInfo("Invalid symbols run start");
            }

            if (MMA1_LIKELY(buffer_[buffer_size - 1].symbol == data_sym_))
            {
                suffix_size_ = 2;
            }
            else {
                suffix_size_ = 1;
            }

            size_t suffix_start = buffer_size - suffix_size_;

            self().process_body(buffer_, prefix_size_, suffix_start);
            self().process_suffix(buffer_, suffix_start);
        }
        else if (MMA1_LIKELY(buffer_size == 2))
        {
            int32_t s0 = buffer_[0].symbol;
            int32_t s1 = buffer_[1].symbol;

            if (s0 == keys_sym_ && s1 == data_sym_)
            {
                prefix_size_ = 0;
                if (s1 == keys_sym_) {
                    suffix_size_ = 1;
                }
                else {
                    suffix_size_ = 2;
                }
            }
            else if (s0 == data_sym_ && s1 == keys_sym_)
            {
                prefix_size_ = 1;
                suffix_size_ = 1;
            }
            else {
                MMA1_THROW(RuntimeException()) << WhatCInfo("Incorrect stream format: double data runs");
            }
        }
        else if (buffer_size == 1)
        {
            int32_t s0 = buffer_[0].symbol;
            if (s0 == keys_sym_)
            {
                prefix_size_ = 0;
                suffix_size_ = 1;
            }
            else {
                prefix_size_ = 1;
                suffix_size_ = 0;
            }
        }
        else {
            prefix_size_ = 0;
            suffix_size_ = 0;
        }

        process_prefix_ = true;
        start_idx_ = 0;
    }

    int32_t start_idx() const {
        return start_idx_;
    }

    bool process_prefix() const {
        return process_prefix_;
    }

    size_t prefix_size() const {
        return prefix_size_;
    }

    size_t suffix_size() const {
        return suffix_size_;
    }

    const io::SymbolsBuffer& buffer() const {
        return buffer_;
    }
};

template <int32_t Streams>
class DefaultBTFLSequenceParser: public BTFLSequenceParserBase<DefaultBTFLSequenceParser<Streams>> {
    using Base = BTFLSequenceParserBase<DefaultBTFLSequenceParser<Streams>>;
public:

    DefaultBTFLSequenceParser(uint64_t start):
        Base(start, Streams)
    {}
};





}
