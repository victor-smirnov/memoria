
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

#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/tools/isequencedata.hpp>
#include <memoria/core/packed/sseq/packed_fse_searchable_seq.hpp>

#include <memoria/core/tools/bitmap.hpp>

#include <iostream>
#include <functional>

namespace memoria {

template <typename Seq>
class SequenceDataSourceAdapter: public ISequenceDataSource<typename Seq::Value, Seq::BitsPerSymbol> {

    static const SizeT Bits     = Seq::BitsPerSymbol;

    typedef typename Seq::Value T;

    SizeT   start0_;
    SizeT   start_;
    SizeT   length_;

    const Seq*  sequence_;
public:

    SequenceDataSourceAdapter(const Seq* sequence, SizeT start, SizeT length):
        start0_(start),
        start_(start),
        length_(length),
        sequence_(sequence)
    {}


    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;
        start_ = length_;

        return distance;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
    }

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual SizeT getAdvance() const
    {
        return getRemainder();
    }

    virtual void  reset(SizeT pos)
    {
        start_  = start0_;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        auto syms = sequence_->symbols();

        MoveBits(syms, buffer, start_ * Bits, start * Bits, length * Bits);

        return skip(length);
    }

    virtual T get()
    {
        T value = sequence_->symbol(start_);

        skip(1);

        return value;
    }
};



template <typename Seq>
class SequenceDataTargetAdapter: public ISequenceDataTarget<typename Seq::Value, Seq::BitsPerSymbol> {

    static const SizeT Bits     = Seq::BitsPerSymbol;

    typedef typename Seq::Value T;

    SizeT   start0_;
    SizeT   start_;
    SizeT   length_;

    Seq*    sequence_;
public:

    SequenceDataTargetAdapter(Seq* sequence, SizeT start, SizeT length):
        start0_(start),
        start_(start),
        length_(length),
        sequence_(sequence)
    {}

    virtual SizeT skip(SizeT length)
    {
        if (start_ + length <= length_)
        {
            start_ += length;
            return length;
        }

        SizeT distance = length_ - start_;
        start_ = length_;

        return distance;
    }

    virtual SizeT getStart() const
    {
        return start_;
    }

    virtual SizeT getRemainder() const
    {
        return length_ - start_;
    }

    virtual SizeT getSize() const
    {
        return length_;
    }

    virtual SizeT getAdvance() const
    {
        return getRemainder();
    }

    virtual void  reset(SizeT pos)
    {
        start_  = start0_;
    }

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        auto syms = sequence_->symbols();

        MoveBits(buffer, syms, start * Bits, start_ * Bits, length * Bits);

        return skip(length);
    }

    virtual void put(const T& value)
    {
        sequence_->symbol(start_) = value;

        skip(1);
    }
};




template <int32_t BitsPerSymbol>
class PackedFSESequence {
protected:
    typedef PackedFSESequence<BitsPerSymbol>                                    MyType;

    typedef typename PkdFSSeqTF<BitsPerSymbol>::Type                            Types;
    typedef PkdFSSeq<Types>                                                     Seq;

    Seq* sequence_;

public:

    typedef typename Seq::Value                                                 Symbol;


    static const int32_t Bits                                                       = Seq::BitsPerSymbol;
    static const int32_t Symbols                                                    = Seq::Indexes;

    typedef ISequenceDataSource<Symbol, Bits>                                   IDataSrc;
    typedef ISequenceDataTarget<Symbol, Bits>                                   IDataTgt;
    typedef SequenceDataSourceAdapter<Seq>                                      SourceAdapter;
    typedef SequenceDataTargetAdapter<Seq>                                      TargetAdapter;

    typedef typename Seq::ConstSymbolAccessor                                   ConstSymbolAccessor;
    typedef typename Seq::SymbolAccessor                                        SymbolAccessor;

    typedef StaticVector<int64_t, Symbols>                                       Ranks;

    PackedFSESequence(int32_t capacity = 1024, int32_t density_hi = 1, int32_t density_lo = 1)
    {
        int32_t sequence_block_size = Seq::estimate_block_size(capacity, density_hi, density_lo);

        int32_t block_size = PackedAllocator::block_size(sequence_block_size, 1);

        PackedAllocator* alloc = allocate_system<PackedAllocator>(block_size).release();
        OOM_THROW_IF_FAILED(alloc, MMA1_SRC);

        OOM_THROW_IF_FAILED(alloc->init(block_size, 1), MMA1_SRC);

        sequence_ = alloc->template allocateEmpty<Seq>(0);

        OOM_THROW_IF_FAILED(sequence_, MMA1_SRC);
    }

    ~PackedFSESequence()
    {
        if (sequence_)
        {
            free_system(sequence_->allocator());
        }
    }

    PackedFSESequence(const MyType& other)
    {
        const PackedAllocator* other_allocator = other.sequence_->allocator();

        int32_t block_size = other_allocator->block_size();

        auto allocator = allocate_system<PackedAllocator>(block_size);

        CopyByteBuffer(other_allocator, allocator.get(), block_size);

        sequence_ = allocator->template get<Seq>(0);
    }

    PackedFSESequence(MyType&& other)
    {
        sequence_ = other.sequence_;
        other.sequence_ = nullptr;
    }

    int32_t size() const
    {
        return sequence_->size();
    }

    void reindex()
    {
        sequence_->reindex();
    }

    void dump(std::ostream& out = std::cout) const
    {
        sequence_->dump(out);
    }

    SymbolAccessor operator[](int32_t idx)
    {
        return sequence_->symbol(idx);
    }

    ConstSymbolAccessor operator[](int32_t idx) const
    {
        return static_cast<const Seq*>(sequence_)->symbol(idx);
    }

    SourceAdapter source(int32_t idx, int32_t length) const
    {
        return SourceAdapter(sequence_, idx, length);
    }

    SourceAdapter source() const
    {
        return source(0, sequence_->size());
    }

    TargetAdapter target(int32_t idx, int32_t length) const
    {
        return TargetAdapter(sequence_, idx, length);
    }

    TargetAdapter target() const
    {
        return target(0, sequence_->size());
    }

    void update(int32_t start, IDataSrc& src)
    {
        Symbol* syms = sequence_->symbols();
        src.get(syms, start, src.getSize());
    }

    void insert(int32_t at, IDataSrc& src)
    {
        //sequence_->insert(&src, at, src.getRemainder());
    }

    void insert(int32_t at, int32_t symbol)
    {
        sequence_->insert(at, symbol);
    }

    void insert(int32_t at, int32_t length, std::function<Symbol ()> fn)
    {
        sequence_->insert(at, length, fn);
    }

    void remove(int32_t start, int32_t end)
    {
        sequence_->remove(start, end);
    }

    void append(IDataSrc& src)
    {
//        int32_t at = sequence_->size();
        //sequence_->insert(&src, at, src.getRemainder());
    }

    void append(int32_t length, std::function<Symbol ()> fn)
    {
        sequence_->insert(sequence_->size(), length, fn);
    }

    void append(Symbol symbol)
    {
        int32_t size = sequence_->size();
        sequence_->insert(size, symbol);
    }

    void read(int32_t from, IDataTgt& tgt) const
    {
        const Symbol* syms = sequence_->symbols();
        tgt.put(syms, from, tgt.getSize());
    }

    int32_t rank(int32_t end, int32_t symbol) const {
        return sequence_->rank(end, symbol);
    }

    int32_t rank(int32_t start, int32_t end, int32_t symbol) const {
        return sequence_->rank(start, end, symbol);
    }

    SelectResult select(int32_t symbol, int32_t rank) const {
        return sequence_->selectFw(symbol, rank);
    }

    SelectResult selectFw(int32_t start, int32_t symbol, int32_t rank) const {
        return sequence_->selectFw(start, symbol, rank);
    }

    SelectResult selectBw(int32_t symbol, int32_t rank) const {
        return sequence_->selectFw(symbol, rank);
    }

    SelectResult selectBw(int32_t start, int32_t symbol, int32_t rank) const {
        return sequence_->selectBw(start, symbol, rank);
    }

    Ranks ranks() const
    {
        return ranks(size());
    }

    Ranks ranks(int32_t to) const
    {
        auto s_ranks = sequence_->sums(to);
        Ranks ranks;
        ranks.assignDown(s_ranks);

        return ranks;
    }

    Ranks ranks(int32_t from, int32_t to) const
    {
        auto s_ranks = sequence_->sums(from, to);
        Ranks ranks;
        ranks.assignDown(s_ranks);

        return ranks;
    }
};

}
