
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/iovector/io_substream.hpp>

#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/span.hpp>

#include <memoria/core/integer/accumulator_common.hpp>
#include <memoria/core/ssrle/ssrle.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <functional>

namespace memoria {
namespace io {


struct IOSSRLEBufferBase: IOSubstream {

    using SeqSizeT = UAcc128T;
    using SymbolT  = size_t;

    virtual bool is_indexed() const                 = 0;
    virtual SymbolT alphabet_size() const           = 0;
    virtual bool is_const() const                   = 0;

    virtual SymbolT symbol(SeqSizeT idx) const      = 0;
    virtual SeqSizeT size() const                   = 0;

    virtual void reindex()                          = 0;
    virtual void dump(std::ostream& out) const      = 0;

    virtual void rank_to(SeqSizeT idx, Span<SeqSizeT> values) const  = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLEBufferBase>::name();
    }

    virtual const std::type_info& sequence_type() const = 0;
    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLEBufferBase);
    }

    virtual void reset()                            = 0;
    virtual void configure(const void* ptr)         = 0;
};


template <size_t Symbols>
class IOSSRLEBuffer: public IOSSRLEBufferBase {
    using Base = IOSSRLEBufferBase;
public:
    using typename Base::SeqSizeT;
    using typename Base::SymbolT;

    static constexpr SymbolT BITS_PER_SYMBOL = BitsPerSymbolConstexpr(Symbols);

    using RunTraits = SSRLERunTraits<BITS_PER_SYMBOL>;
    using RunT      = SSRLERun<BITS_PER_SYMBOL>;
    using CodeUnitT = typename RunTraits::CodeUnitT;

    virtual void append(Span<const RunT> runs) = 0;
    virtual Span<const CodeUnitT> code_units() const = 0;
    virtual std::vector<RunT> symbol_runs(SeqSizeT start = SeqSizeT{}, SeqSizeT size = SeqSizeT::max()) const = 0;
};


std::unique_ptr<IOSSRLEBufferBase> make_packed_ssrle_buffer(size_t alphabet_size);

}}
