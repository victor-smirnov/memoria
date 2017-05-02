
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

#include <memoria/v1/core/tools/bitmap_select.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include "rleseq_tools.hpp"

namespace memoria {
namespace v1 {



namespace rleseq {

template <typename Seq>
class RLESeqIterator {
    using Codec = typename Seq::Codec;
    using Value = typename Seq::Value;

    const Value* symbols_;

    size_t data_pos_;
    size_t data_size_;

    size_t run_prefix_;
    size_t local_idx_;

    Codec codec_;

    RLESymbolsRun run_;

    RLESymbolsRun run_backup_;
    size_t datapos_backup_      = 0;
    size_t run_prefix_backup_   = 0;
    size_t local_idx_backup_    = 0;

public:
    RLESeqIterator(): symbols_(), data_pos_(), data_size_(), run_prefix_(), local_idx_() {}
    RLESeqIterator(const Value* symbols, size_t data_pos, size_t data_size, size_t local_idx, size_t run_prefix, RLESymbolsRun run):
        symbols_(symbols),
        data_pos_(data_pos),
        data_size_(data_size),
        run_prefix_(run_prefix),
        local_idx_(local_idx),
        run_(run)
    {
        if (data_pos < data_size)
        {
            Codec codec;
            data_pos_ += codec.length(Seq::encode_run(run.symbol(), run.length()));
        }
    }


    bool has_symbol_in_run() const {
        return local_idx_ < run_.length();
    }

    bool has_data() const {
        return has_symbol_in_run() || data_pos_ < data_size_;
    }

    size_t local_idx() const {
        return local_idx_;
    }

    size_t length() const {
        return run_.length();
    }

    const auto& run() const {
        return run_;
    }

    size_t data_pos() const {return data_pos_;}
    size_t data_size() const {return data_size_;}

    size_t idx() const {return run_prefix_ + local_idx();}

    int32_t symbol() const {return run_.symbol();}

    bool is_found() const {
        return has_data();
    }

    void next_run()
    {
        if (data_pos_ < data_size_)
        {
            run_prefix_ += run_.length();

            local_idx_ = 0;

            uint64_t value = 0;
            auto len = codec_.decode(symbols_, value, data_pos_);

            run_ = Seq::decode_run(value);

            data_pos_ += len;
        }
        else {
            local_idx_ = run_.length();
        }
    }

    int32_t next_symbol_in_run()
    {
        if (local_idx_ < run_.length() - 1)
        {
            int32_t sym = run_.symbol();
            local_idx_++;

            return sym;
        }
    }

    void next()
    {
        if (++local_idx_ < run_.length())
        {
            return;
        }

        next_run();
    }

    void mark()
    {
        run_backup_         = run_;
        run_prefix_backup_  = run_prefix_;
        local_idx_backup_   = local_idx_;
        datapos_backup_     = data_pos_;
    }


    void restore()
    {
        run_        = run_backup_;
        run_prefix_ = run_prefix_backup_;
        local_idx_  = local_idx_backup_;
        data_pos_   = datapos_backup_;
    }
};

}}}
