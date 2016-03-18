
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP

#include <memoria/core/tools/isymbols.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/core/packed/wrappers/symbol_sequence.hpp>

#include <memoria/prototypes/bt/layouts/bt_input.hpp>

namespace memoria       {
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
class SequenceInputProviderBase: public memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
    using Base = memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

    using typename Base::InputBuffer;

    using SequenceInputBuffer = typename InputBuffer::template StreamTypeT<0>;

public:
    SequenceInputProviderBase(CtrT& ctr, Int capacity = 10000):
        Base(ctr, capacity)
    {}

    virtual Int get(InputBuffer* buffer, Int pos) {
        return get(buffer->template substream_by_idx<0>(), pos);
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

    using typename Base::SequenceInputBuffer;
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
}

#endif
