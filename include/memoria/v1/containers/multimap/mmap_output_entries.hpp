
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

template <typename Types, typename Profile, typename IteratorPtr>
class EntriesIteratorImpl: public IEntriesIterator<Types, Profile> {
    using Base = IEntriesIterator<Types, Profile>;

    using Base::parser_;
    using Base::offsets_;
    using Base::prefix_;
    using Base::suffix_key_;
    using Base::suffix_;
    using Base::has_suffix_;
    using Base::iteration_num_;

    using Base::keys_;
    using Base::values_;

    using typename Base::KeyView;
    using typename Base::Key;
    using typename Base::ValueView;
    using typename Base::Value;

    using typename Base::KeysIOVSubstreamAdapter;
    using typename Base::ValuesIOVSubstreamAdapter;



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
        if (iter_->nextLeaf())
        {
            offsets_.clear();
            build_index();
        }
        else {
            iter_->local_pos() = iter_->leaf_sizes().sum();
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

        parser_.parse(ss);

        size_t prefix_size;

        if (MMA1_LIKELY(parser_.prefix_size() > 0))
        {
            prefix_size = parser_.buffer()[0].length;
            ValuesIOVSubstreamAdapter::read_to(buffer.substream(1), 0, 0, prefix_size, prefix_.array());
        }
        else {
            prefix_.clear();
            prefix_size = 0;
        }

        keys_.clear();
        values_.clear();

        auto key_offset = offsets_[0];
        auto value_offset = offsets_[1] + prefix_size;

        size_t body_end = parser_.buffer().size() - parser_.suffix_size();


        size_t values_end;
        size_t num_keys;

        std::tie(values_end, num_keys) = values_.template populate<ValuesIOVSubstreamAdapter>(
            buffer.substream(1),
            parser_,
            value_offset,
            parser_.prefix_size(), body_end
        );

        KeysIOVSubstreamAdapter::read_to(buffer.substream(0), 0, key_offset, num_keys, keys_.array());



        // suffix processing

        has_suffix_ = true;

        if (parser_.suffix_size() == 2)
        {
            size_t suffix_size = parser_.buffer()[body_end + 1].length;

            KeysIOVSubstreamAdapter::read_one(buffer.substream(0), 0, key_offset + num_keys, suffix_key_);
            ValuesIOVSubstreamAdapter::read_to(buffer.substream(1), 0, values_end, suffix_size, suffix_.array());
        }
        else if (parser_.suffix_size() == 1)
        {
            KeysIOVSubstreamAdapter::read_one(buffer.substream(0), 0, key_offset + num_keys, suffix_key_);
            suffix_.clear();
        }
        else {
            suffix_.clear();
            has_suffix_ = false;
        }

        iteration_num_++;
    }
};

}
}}
