
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

#include <memoria/v1/core/tools/isymbols.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/packed/wrappers/symbol_sequence.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>

namespace memoria {
namespace v1 {
namespace seq_dense     {


template <Int Bits>
class SymbolsInputBufferProvider: public bt::InputBufferProvider<Int, SymbolsBuffer<Bits>> {

    using Buffer = SymbolsBuffer<Bits>;

    using Position = Int;

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
class SequenceInputProviderBase: public v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
    using Base = v1::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

    using InputBuffer = typename Base::InputBuffer;

    using SequenceInputBuffer = typename InputBuffer::template StreamTypeT<1>;

public:
    SequenceInputProviderBase(CtrT& ctr, Int capacity = 10000):
        Base(ctr, capacity)
    {}

    virtual Int get(InputBuffer* buffer, Int pos)
    {
        Int inserted = get(buffer->template substream_by_idx<1>(), pos);

        buffer->template substream_by_idx<0>()->append(inserted, 0);

        return inserted;
    }

    virtual Int get(SequenceInputBuffer* buffer, Int pos) = 0;
};

template <typename CtrT>
class FnSequenceInputProvider: public SequenceInputProviderBase<CtrT> {
    using Base = SequenceInputProviderBase<CtrT>;

public:

    using typename Base::SequenceInputBuffer;

public:
    FnSequenceInputProvider(CtrT& ctr, Int capacity = 10000):
        Base(ctr, capacity)
    {}

    virtual Int get(SequenceInputBuffer* buffer, Int pos)
    {
        return 0;
    }
};


template <typename CtrT, typename RngT = RngBigInt>
class RandomSequenceInputProvider: public SequenceInputProviderBase<CtrT> {
    using Base = SequenceInputProviderBase<CtrT>;

    RngT& rng_;

public:

    using typename Base::SequenceInputBuffer;

    BigInt total_ = 0;
    BigInt length_;

public:
    RandomSequenceInputProvider(CtrT& ctr, RngT& rng, BigInt length, Int capacity = 10000):
        Base(ctr, capacity),
        rng_(rng),
        length_(length)
    {}

    virtual Int get(SequenceInputBuffer* buffer, Int pos)
    {
        using SymbolsBuffer = typename SequenceInputBuffer::SymbolsBuffer;

        if (total_ < length_)
        {
            return buffer->append([&, this](Int size) {

                SymbolsBuffer buf;
                Int limit = size > buf.capacity() ? buf.capacity() : size;

                if (total_ + limit > length_) {
                    limit = length_ - total_;
                }

                buf.resize(limit);

                auto symbols = buf.symbols();

                for (Int c = 0; c < SymbolsBuffer::BufSize; c++)
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

    static constexpr Int BitsPerSymbol = CtrT::Types::BitsPerSymbol;
private:

    const Symbols* symbols_;

    Int start_;
    Int length_;

public:
    SymbolSequenceInputProvider(CtrT& ctr, const Symbols* symbols, Int start, Int length, Int capacity = 100000):
        Base(ctr, capacity),
        symbols_(symbols),
        start_(start),
        length_(length)
    {}

    virtual Int get(SequenceInputBuffer* buffer, Int pos)
    {
        if (start_ < length_)
        {
            Int inserted = buffer->append(symbols_, start_, length_);
            start_ += inserted;
            return inserted;
        }
        else {
            return -1;
        }
    }
};


}
}}
