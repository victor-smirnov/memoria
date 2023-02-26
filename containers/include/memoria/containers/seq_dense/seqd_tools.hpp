
// Copyright 2013 Victor Smirnov
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

#include <memoria/core/tools/isymbols.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/core/packed/wrappers/symbol_sequence.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/prototypes/bt_ss/btss_input.hpp>

namespace memoria {
namespace seq_dense {


template <int32_t Bits>
class SymbolsInputBufferProvider: public bt::InputBufferProvider<int32_t, SymbolsBuffer<Bits>> {

    using Buffer = SymbolsBuffer<Bits>;

    using Position = int32_t;

    SymbolsBuffer<Bits>& data_;
    Position start_ = 0;
    Position size_ = 0;

    bool next_ = true;
    Position next_size_;

public:


    SymbolsInputBufferProvider(Buffer& data, Position start = 0): data_(data), next_size_(data.size()) {}
    SymbolsInputBufferProvider(Buffer& data, Position start, Position size): data_(data), next_size_(size)  {}

    virtual Position start() const {
        return start_;
    }

    virtual Position size() const {
        return size_;
    }

    virtual Position zero() const {return 0;}

    virtual const Buffer* buffer() const {
        return &data_;
    }

    virtual void consumed(Position sizes) {
        start_ += sizes;
    }

    virtual bool isConsumed() {
        return start_ >= size_;
    }

    virtual void nextBuffer()
    {
        if (next_) {
            next_ = false;
            size_ = next_size_;
            start_ = 0;
        }
        else {
            size_ = 0;
            start_ = 0;
        }
    }

    virtual bool hasData() const {
        return next_ || start_ < size_;
    }
};


template <typename CtrT>
class SequenceInputProviderBase: public btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
    using Base = btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

    using InputBuffer = typename Base::InputBuffer;

    using SequenceInputBuffer = typename InputBuffer::template StreamTypeT<1>;

public:
    SequenceInputProviderBase(CtrT& ctr, int32_t capacity = 10000):
        Base(ctr, capacity)
    {}

    virtual int32_t get(InputBuffer* buffer, int32_t pos)
    {
        int32_t inserted = get(buffer->template substream_by_idx<1>(), pos);

        buffer->template substream_by_idx<0>()->append(inserted, 0);

        return inserted;
    }

    virtual int32_t get(SequenceInputBuffer* buffer, int32_t pos) = 0;
};

template <typename CtrT>
class FnSequenceInputProvider: public SequenceInputProviderBase<CtrT> {
    using Base = SequenceInputProviderBase<CtrT>;

public:

    using typename Base::SequenceInputBuffer;

public:
    FnSequenceInputProvider(CtrT& ctr, int32_t capacity = 10000):
        Base(ctr, capacity)
    {}

    virtual int32_t get(SequenceInputBuffer* buffer, int32_t pos)
    {
        return 0;
    }
};


template <typename CtrT, typename RngT = RngInt64>
class RandomSequenceInputProvider: public SequenceInputProviderBase<CtrT> {
    using Base = SequenceInputProviderBase<CtrT>;

    RngT& rng_;

public:

    using typename Base::SequenceInputBuffer;

    int64_t total_ = 0;
    int64_t length_;

public:
    RandomSequenceInputProvider(CtrT& ctr, RngT& rng, int64_t length, int32_t capacity = 10000):
        Base(ctr, capacity),
        rng_(rng),
        length_(length)
    {}

    virtual int32_t get(SequenceInputBuffer* buffer, int32_t pos)
    {
        using SymbolsBuffer = typename SequenceInputBuffer::SymbolsBuffer;

        if (total_ < length_)
        {
            return buffer->append([&, this](int32_t size) {

                SymbolsBuffer buf;
                int32_t limit = size > buf.capacity() ? buf.capacity() : size;

                if (total_ + limit > length_) {
                    limit = length_ - total_;
                }

                buf.resize(limit);

                auto symbols = buf.symbols();

                for (int32_t c = 0; c < SymbolsBuffer::BufSize; c++)
                {
                    symbols[c] = this->rng_();
                }

                this->total_ += limit;

                return buf;
            });
        }
        else {
            return -1;
        }
    }
};


template <typename CtrT>
class SymbolSequenceInputProvider: public SequenceInputProviderBase<CtrT> {
    using Base = SequenceInputProviderBase<CtrT>;

public:

    using SequenceInputBuffer = typename Base::SequenceInputBuffer;
    using Symbols = typename SequenceInputBuffer::Value;

    static constexpr int32_t BitsPerSymbol = CtrT::Types::BitsPerSymbol;
private:

    const Symbols* symbols_;

    int32_t start_;
    int32_t length_;

public:
    SymbolSequenceInputProvider(CtrT& ctr, const Symbols* symbols, int32_t start, int32_t length, int32_t capacity = 100000):
        Base(ctr, capacity),
        symbols_(symbols),
        start_(start),
        length_(length)
    {}

    virtual int32_t get(SequenceInputBuffer* buffer, int32_t pos)
    {
        if (start_ < length_)
        {
            int32_t inserted = buffer->append(symbols_, start_, length_);
            start_ += inserted;
            return inserted;
        }
        else {
            return -1;
        }
    }
};


}}
