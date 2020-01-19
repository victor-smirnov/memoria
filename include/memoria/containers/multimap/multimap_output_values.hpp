
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
class ValuesIteratorImpl: public IValuesScanner<Types, Profile> {
    using Base = IValuesScanner<Types, Profile>;

    using typename Base::ValueView;
    using typename Base::ValuesIOVSubstreamAdapter;

    using Base::values_;
    using Base::size_;
    using Base::run_is_finished_;
    using Base::values_buffer_;

    size_t values_start_{};

    core::StaticVector<uint64_t, 2> offsets_;
    DefaultBTFLRunParser<2> parser_;

    IteratorPtr iter_;

    uint64_t iteration_num_{};
public:
    ValuesIteratorImpl(IteratorPtr iter):
        parser_(iter->iter_local_pos()),
        iter_(iter)
    {
        parse_first();
    }

    virtual bool is_end() const {
        return iter_->iter_is_end();
    }

    virtual BoolResult next_block() noexcept
    {
        auto res = iter_->iter_next_leaf();
        MEMORIA_RETURN_IF_ERROR(res);

        if (res.get())
        {
            offsets_.clear();
            build_index();
        }
        else {
            iter_->iter_local_pos() = iter_->iter_leaf_sizes().sum();
            run_is_finished_ = true;
        }

        return BoolResult::of(run_is_finished_);
    }

    virtual VoidResult fill_suffix_buffer() noexcept
    {
        while (!is_end())
        {
            fill_buffer(values_start_, values_.size());

            auto res = next_block();
            MEMORIA_RETURN_IF_ERROR(res);

            if (res.get()) {
                break;
            }
        }

        return VoidResult::of();
    }


    virtual void dump_iterator() const
    {
        iter_->dump();
    }

private:

    void fill_buffer(size_t start, size_t end)
    {
        const io::IOVector& buffer = iter_->iovector_view();
        values_buffer_.template append<ValuesIOVSubstreamAdapter>(buffer.substream(1), 0, start, end);
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
        const io::IOVector& buffer = iter_->iovector_view();

        auto& ss = buffer.symbol_sequence();

        parser_.parse(ss);

        if (MMA1_LIKELY((!parser_.is_empty()) || ss.size() > 0))
        {
            size_t run_size = (!parser_.is_empty()) ? parser_.run_size() : 0;

            values_start_ = offsets_[1];
            values_.clear();
            ValuesIOVSubstreamAdapter::read_to(buffer.substream(1), 0, values_start_, run_size, values_.array());

            run_is_finished_ = parser_.is_finished();

            size_ = run_size;
        }
        else {
            // empty block requires nothing
        }

        offsets_.clear();

        iteration_num_++;
    }
};

}}
