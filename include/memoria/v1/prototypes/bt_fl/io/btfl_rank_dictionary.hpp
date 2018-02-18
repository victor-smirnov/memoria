
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_input_tools.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {



template <int32_t Symbols>
class RankDictionary {
    FreeUniquePtr<PackedAllocator>  ref_;

    int32_t last_symbol_    = -1;
    uint64_t run_length_ = 0;

public:
    using Sequence  = PkdRLESeqT<Symbols>;
    using SeqT      = Sequence;


    RankDictionary(int32_t capacity): ref_(allocate(capacity)){}

    SeqT* get() {return ref_->template get<SeqT>(0);}
    const SeqT* get() const {return ref_->template get<SeqT>(0);}

    SeqT* operator->() {return this->get();}
    const SeqT* operator->() const {return this->get();}

    void enlarge(int32_t required = 0)
    {
        int32_t current_size = get()->symbols_block_size();
        int32_t new_size     = current_size * 2;

        int32_t new_capacity = new_size - current_size;
        if (new_capacity < required)
        {
            new_size += required - new_capacity;
        }

        auto new_ptr = allocate(new_size);

        get()->splitTo(new_ptr->template get<SeqT>(0), 0);

        ref_.reset(new_ptr.release());
    }

    void prepare()
    {
        last_symbol_ = -1;
        run_length_  = 0;

        get()->reset();
    }

    void reindex()
    {
        if (last_symbol_ >= 0)
        {
            flush_run();
            last_symbol_ = -1;
        }

        get()->reindex();
    }

    void append_run(int32_t symbol, uint64_t run_length)
    {
        if (symbol == last_symbol_ || last_symbol_ < 0)
        {
            last_symbol_ = symbol;
            run_length_ += run_length;
        }
        else
        {
            flush_run();

            last_symbol_ = symbol;
            run_length_  = run_length;
        }
    }

    void dump(std::ostream& out = std::cout) const {
        get()->dump(out);
    }

private:
    void flush_run()
    {
        if (run_length_ > 0 && !get()->emplace_back(last_symbol_, run_length_))
        {
            enlarge();

            if (!get()->emplace_back(last_symbol_, run_length_))
            {
                MMA1_THROW(Exception()) << WhatCInfo("Symbols run entry is too large for RLE Sequence");
            }
        }
    }


    static auto allocate(int32_t capacity)
    {
        int32_t block_size = SeqT::block_size(capacity);
        auto ptr = AllocTool<PackedAllocator>::create(block_size, 1);

        SeqT* seq = ptr->template allocate<SeqT>(0, block_size);

        seq->enlargeData(capacity);

        return ptr;
    }
};



}}}}
