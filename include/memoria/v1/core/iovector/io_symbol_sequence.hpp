
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/iovector/io_symbol_sequence_base.hpp>
#include <memoria/v1/core/iovector/io_packed_symbol_sequence_owning.hpp>
#include <memoria/v1/core/iovector/io_packed_symbol_sequence_non_owning.hpp>

#include <functional>

namespace memoria {
namespace v1 {
namespace io {

class IOSymbolSequence {
    std::unique_ptr<ISymbolSequence> sequence_;
public:
    IOSymbolSequence(std::unique_ptr<ISymbolSequence>&& sequence):
        sequence_(std::move(sequence))
    {}


    bool is_indexed() const {
        return sequence_->is_indexed();
    }

    int32_t alphabet_size() const {
        return sequence_->alphabet_size();
    }

    bool is_const() const
    {
        return sequence_->is_const();
    }

    int32_t symbol(int32_t idx) const
    {
        return sequence_->symbol(idx);
    }

    int32_t size() const {
        return sequence_->size();
    }

    void* buffer() const {
        return sequence_->buffer();
    }

    void reindex() {
        return sequence_->reindex();
    }

    void dump(std::ostream& out) const {
        sequence_->dump(out);
    }

    void rank_to(int32_t idx, int32_t* values) const
    {
        sequence_->rank_to(idx, values);
    }

    void append(int32_t symbol, int32_t length) {
        sequence_->append(symbol, length);
    }

    void reset() {
        sequence_->reset();
    }

    const std::type_info& sequence_type() const {
        return sequence_->sequence_type();
    }
};

}}}
