
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

template <typename Key, typename Value, typename IteratorPtr>
class EntriesIteratorImpl: public IEntriesIterator<Key, Value> {
    using Base = IEntriesIterator<Key, Value>;

    using Base::parser_;
    using Base::offsets_;
    using Base::prefix_;
    using Base::prefix_size_;
    using Base::suffix_key_;
    using Base::suffix_key_ptr_;
    using Base::suffix_;
    using Base::suffix_size_;
    using Base::has_suffix_;
    using Base::body_;
    using Base::use_buffered_;
    using Base::suffix_buffer_;
    using Base::buffer_is_ready_;
    using Base::iteration_num_;

    using typename Base::Entry;

    IteratorPtr iter_;
public:
    EntriesIteratorImpl(IteratorPtr iter):
        Base(iter->local_pos()),
        iter_(iter)
    {
        parse_first();
    }

    virtual bool is_end() const {
        return iter_->isEnd();
    }

    virtual void next()
    {
        if (use_buffered_)
        {
            fill_buffer_suffix();
        }

        if (iter_->nextLeaf())
        {
            offsets_.clear();
            build_index();
        }
        else {
            iter_->local_pos() = iter_->leaf_sizes().sum();
            buffer_is_ready_ = true;
        }
    }

    virtual void set_buffered()
    {
        if (!use_buffered_)
        {
            if (iteration_num_ == 1)
            {
                use_buffered_ = true;
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

    void fill_buffer_suffix()
    {
        if (parser_.suffix_size() > 0)
        {
            suffix_buffer_.clear();
            buffer_is_ready_ = false;
        }

        has_suffix_ = false;

        if (parser_.suffix_size() == 2)
        {
            suffix_key_ = *suffix_key_ptr_;
            size_t suffix_size_idx = parser_.buffer().size() - 1;
            suffix_size_ = parser_.buffer()[suffix_size_idx].length;
            suffix_buffer_.append_values(suffix_, suffix_size_);
        }
        else if (parser_.suffix_size() == 1)
        {
            suffix_key_ = *suffix_key_ptr_;
            suffix_size_ = 0;
            suffix_ = nullptr;
        }
        else {
            suffix_size_ = 0;
            suffix_ = nullptr;
        }
    }

    void build_index()
    {
        //this->dump_iterator();

        auto& buffer = iter_->iovector_view();

        auto& ss = buffer.symbol_sequence();
        auto& s0 = io::substream_cast<const io::IOColumnwiseFixedSizeArraySubstream<Key>>(buffer.substream(0));
        auto& s1 = io::substream_cast<const io::IORowwiseFixedSizeArraySubstream<Value>>(buffer.substream(1));

        parser_.parse(ss);

        if (MMA1_LIKELY(parser_.prefix_size() > 0))
        {
            prefix_ = s1.select(0);
            prefix_size_ = parser_.buffer()[0].length;

            if (use_buffered_)
            {
                suffix_buffer_.append_values(prefix_, prefix_size_);
            }
        }
        else {
            prefix_ = nullptr;
            prefix_size_ = 0;
        }

        body_.clear();

        auto key_offset = offsets_[0];
        auto value_offset = offsets_[1] + prefix_size_;

        const Key* keys = s0.select(0, key_offset);
        const Value* values = s1.select(value_offset);

        size_t body_end = parser_.buffer().size() - parser_.suffix_size();
        for (size_t c = parser_.prefix_size(); c < body_end;)
        {
            uint64_t keys_run_length = parser_.buffer()[c].length;
            if (MMA1_LIKELY(keys_run_length == 1)) // typical case
            {
                uint64_t values_run_length = parser_.buffer()[c + 1].length;
                body_.append_value(Entry{keys, absl::Span<const Value>(values, values_run_length)});

                keys++;
                values += values_run_length;
                c += 2;
            }
            else
            {
                // zero-length values
                for (uint64_t s = 0; s < keys_run_length; s++)
                {
                    body_.append_value(Entry{keys, absl::Span<const Value>(nullptr, 0)});
                    keys++;
                }

                c += 1;
            }
        }

        // suffix processing

        suffix_key_ptr_ = keys;
        suffix_ = values;

        if (!use_buffered_)
        {
            has_suffix_ = true;

            if (parser_.suffix_size() == 2)
            {
                suffix_key_ = *keys;
                suffix_size_ = parser_.buffer()[body_end + 1].length;
            }
            else if (parser_.suffix_size() == 1)
            {
                suffix_key_ = *keys;
                suffix_size_ = 0;
                suffix_ = nullptr;
            }
            else {
                suffix_size_ = 0;
                suffix_ = nullptr;
                has_suffix_ = false;
            }
        }
        else {
            if (parser_.suffix_size() > 0)
            {
                buffer_is_ready_ = true;
            }
        }

        iteration_num_++;
    }
};

}
}}
