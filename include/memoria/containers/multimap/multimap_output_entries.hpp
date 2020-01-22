
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

#include <memoria/api/multimap/multimap_output.hpp>

namespace memoria {
namespace multimap {

template <typename Types, typename Profile, typename IteratorPtr>
class EntriesIteratorImpl: public IEntriesScanner<Types, Profile> {
    using Base = IEntriesScanner<Types, Profile>;

    using Base::parser_;
    using Base::offsets_;
    using Base::prefix_;
    using Base::suffix_key_;
    using Base::suffix_;
    using Base::suffix_buffer_;
    using Base::has_suffix_;
    using Base::iteration_num_;

    using Base::keys_;
    using Base::values_;

    using Base::has_suffix;

    using typename Base::KeyView;
    using typename Base::ValueView;

    using typename Base::KeysIOVSubstreamAdapter;
    using typename Base::ValuesIOVSubstreamAdapter;

    size_t suffix_start_{};

    IteratorPtr iter_;
public:
    EntriesIteratorImpl(IteratorPtr iter):
        Base(iter->iter_local_pos()),
        iter_(iter)
    {
        parse_first();
    }

    virtual bool is_end() const noexcept {
        return iter_->iter_is_end();
    }

    virtual BoolResult next() noexcept
    {
        auto res0 = iter_->iter_next_leaf();
        MEMORIA_RETURN_IF_ERROR(res0);

        if (res0.get())
        {
            offsets_.clear();
            build_index();
            return BoolResult::of(true);
        }
        else {
            iter_->iter_local_pos() = iter_->iter_leaf_sizes().sum();
            return BoolResult::of(false);
        }
    }

    virtual void dump_iterator() const
    {
        iter_->dump();

        std::cout << "Buffer: " << std::endl;

        size_t cnt{};
        for (auto& entry: parser_.buffer().span()) {
            std::cout << cnt << ") " << entry.symbol << " :: " << entry.length << std::endl;
            cnt++;
        }
    }

    virtual VoidResult fill_suffix_buffer() noexcept
    {
        suffix_buffer_.clear();

        fill_buffer(suffix_start_, suffix_.size());

        while (!is_end())
        {
            auto res = next();
            MEMORIA_RETURN_IF_ERROR(res);

            if (res.get())
            {
                fill_buffer(0, prefix_.size());

                if (has_suffix()) {
                    break;
                }
            }
        }

        return VoidResult::of();
    }

private:

    void fill_buffer(size_t start, size_t end)
    {
        const io::IOVector& buffer = iter_->iovector_view();
        suffix_buffer_.template append<ValuesIOVSubstreamAdapter>(buffer.substream(1), 0, start, end);
    }

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

        parser_.parse(ss);

        prefix_.clear();

        if (MMA_LIKELY(parser_.prefix_size() > 0))
        {
            size_t prefix_size = parser_.buffer()[0].length;
            ValuesIOVSubstreamAdapter::read_to(buffer.substream(1), 0, 0, prefix_size, prefix_.array());
        }


        keys_.clear();
        values_.clear();

        auto key_offset = offsets_[0];
        auto value_offset = offsets_[1] + prefix_.size();

        size_t body_end = parser_.buffer().size() - parser_.suffix_size();
        size_t num_keys;

        std::tie(suffix_start_, num_keys) = values_.template populate<ValuesIOVSubstreamAdapter>(
            buffer.substream(1),
            parser_,
            value_offset,
            parser_.prefix_size(), body_end
        );

        KeysIOVSubstreamAdapter::read_to(buffer.substream(0), 0, key_offset, num_keys, keys_.array());

        // suffix processing

        has_suffix_ = true;

        suffix_.clear();

        if (parser_.suffix_size() == 2)
        {
            size_t suffix_size = parser_.buffer()[body_end + 1].length;

            KeysIOVSubstreamAdapter::read_one(buffer.substream(0), 0, key_offset + num_keys, suffix_key_);
            ValuesIOVSubstreamAdapter::read_to(buffer.substream(1), 0, suffix_start_, suffix_size, suffix_.array());
        }
        else if (parser_.suffix_size() == 1)
        {
            KeysIOVSubstreamAdapter::read_one(buffer.substream(0), 0, key_offset + num_keys, suffix_key_);
        }
        else {
            has_suffix_ = false;
        }

        iteration_num_++;
    }
};

}}
