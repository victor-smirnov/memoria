
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

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <functional>

namespace memoria {
namespace v1 {
namespace io {

template <int32_t AlphabetSize>
class PackedRLESymbolSequence;

template <>
class PackedRLESymbolSequence<1>: public IOSymbolSequence {

    int32_t size_{};

public:
    PackedRLESymbolSequence()
    {
    }

    PackedRLESymbolSequence(PackedRLESymbolSequence&&) = delete;
    PackedRLESymbolSequence(const PackedRLESymbolSequence&) = delete;


    virtual bool is_indexed() const {
        return true;
    }

    virtual int32_t alphabet_size() const {
        return 1;
    }

    virtual bool is_const() const {
        return false;
    }

    virtual int32_t symbol(int32_t idx) const {
        return 0;
    }

    virtual int32_t size() const {
        return size_;
    }

    virtual void* buffer() const {
        return nullptr;
    }

    virtual void rank_to(int32_t idx, int32_t* values) const
    {
        values[0] = idx;
    }

    virtual void append(int32_t symbol, int32_t length)
    {
        size_ += length;
    }

    virtual void reindex()
    {
    }

    virtual void dump(std::ostream& out) const
    {
        out << "[SingleBitSymbolSequence: " << size_ << "]";
    }

    virtual void reset() {
        size_ = 0;
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(PackedRLESymbolSequence<1>);
    }

    virtual void configure(void* ptr) {
        MMA1_THROW(UnsupportedOperationException());
    }
};




}}}
