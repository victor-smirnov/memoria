
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

#include <memoria/v1/api/multimap/multimap_output.hpp>

namespace memoria {
namespace v1 {
namespace mmap {

template <typename Value, typename IteratorPtr>
class ValuesIteratorImpl: public IValuesIterator<Value> {
    using Base = IValuesIterator<Value>;

    using Base::values_;
    using Base::size_;
    using Base::use_buffered_;
    using Base::buffer_is_ready_;


    core::StaticVector<uint64_t, 2> offsets_;
    DefaultBTFLRunParser<2> parser_;
    io::DefaultIOBuffer<Value> values_buffer_{128};

    IteratorPtr iter_;

    uint64_t iteration_num_{};
public:
    ValuesIteratorImpl(IteratorPtr iter):
        parser_(iter->local_pos()),
        iter_(iter)
    {
        parse_first();
    }

    virtual bool is_end() const {
        return iter_->isEnd();
    }

    virtual void next()
    {
        if (iter_->nextLeaf())
        {
            offsets_.clear();
            build_index();
        }
        else {
            iter_->local_pos() = iter_->leaf_sizes().sum();
            buffer_is_ready_ = true;

            if (use_buffered_)
            {
                values_ = values_buffer_.tail_ptr();
            }
        }
    }

    virtual void set_buffered()
    {
        if (!use_buffered_)
        {
            if (iteration_num_ == 1)
            {
                use_buffered_ = true;
                if (!buffer_is_ready_)
                {
                    values_buffer_.append_values(values_, size_);
                }
            }
            else {
                MMA1_THROW(RuntimeException()) << WhatCInfo("set_buffered() must be invoked before next()");
            }
        }
    }

    virtual void dump_iterator() const
    {
        iter_->dump();
    }

private:

    void parse_first()
    {
        auto& ss = iter_->iovector_view().symbol_sequence();

        int32_t idx = parser_.start_idx();
        ss.rank_to(idx, offsets_.values());

        build_index();
    }

    void build_index()
    {
        auto& buffer = iter_->iovector_view();

        auto& ss = buffer.symbol_sequence();
        auto& s1 = io::substream_cast<const io::IORowwiseFixedSizeArraySubstream<Value>>(buffer.substream(1));

        parser_.parse(ss);

        if (MMA1_LIKELY((!parser_.is_empty()) || ss.size() > 0))
        {
            const Value* ptr = s1.select(offsets_[1]);

            size_t run_size = (!parser_.is_empty()) ? parser_.run_size() : 0;

            buffer_is_ready_ = parser_.is_finished();

            if (use_buffered_)
            {
                values_buffer_.append_values(ptr, run_size);

                if (buffer_is_ready_)
                {
                    values_ = values_buffer_.tail_ptr();
                    size_ = values_buffer_.size();
                }
                else {
                    size_ = run_size;
                }
            }
            else {
                values_ = ptr;
                size_ = run_size;
            }
        }
        else {
            // empty block requires nothing
        }

        offsets_.clear();

        iteration_num_++;
    }
};

}
}}
