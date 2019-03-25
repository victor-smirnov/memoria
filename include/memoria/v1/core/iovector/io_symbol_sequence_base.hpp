
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

#include <memoria/v1/core/iovector/io_substream_base.hpp>

#include <functional>

namespace memoria {
namespace v1 {
namespace io {

struct IOSymbolSequence: IOSubstream {

    virtual bool is_indexed() const                 = 0;
    virtual int32_t alphabet_size() const           = 0;
    virtual bool is_const() const                   = 0;

    virtual int32_t symbol(int32_t idx) const       = 0;
    virtual int32_t size() const                    = 0;
    virtual void* buffer() const                    = 0;
    virtual void reindex()                          = 0;
    virtual void dump(std::ostream& out) const      = 0;

    virtual void rank_to(int32_t idx, int32_t* values) const  = 0;
    virtual void append(int32_t symbol, int32_t length)       = 0;
    virtual const std::type_info& sequence_type() const       = 0;

    virtual void reset() = 0;

    virtual const std::type_info& substream_type() const {
        return typeid(IOSymbolSequence);
    }

    virtual void configure(void* ptr) = 0;
};



}}}
