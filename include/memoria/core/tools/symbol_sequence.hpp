
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_SYMBOLSEQUENCE_HPP
#define _MEMORIA_CORE_TOOLS_SYMBOLSEQUENCE_HPP


#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/isequencedata.hpp>
#include <memoria/core/packed/packed_seq.hpp>

#include <malloc.h>

namespace memoria    {
namespace vapi       {


template <typename Seq>
class SequenceDataSourceAdapter: public ISequenceDataSource<typename Seq::Symbol, Seq::Bits> {

	static const SizeT BITSIZE  = TypeBitsize<typename Seq::Symbol>();
	static const SizeT Bits		= Seq::Bits;

	typedef typename Seq::Symbol T;

	SizeT   start_;
	SizeT   length_;


	const Seq*	sequence_;
	SizeT   idx0_;
	SizeT	idx_;
	SizeT 	bits_;
	SizeT	prefix_;

public:

	SequenceDataSourceAdapter(const Seq* sequence, SizeT idx, SizeT bits):
		start_(0),
		length_(0),
		sequence_(sequence),
		idx0_(idx), idx_(idx),
		bits_(bits),
		prefix_(0)
	{}

	virtual SizeT skip(SizeT length)
	{
		if (start_ + length <= length_)
		{
			start_ += length;
			idx_ += length * BITSIZE;

			return length;
		}

		SizeT distance = length_ - start_;
		start_ = length_;

		idx_ += length * BITSIZE;

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

	virtual void  reset()
	{
		start_ 	= 0;
		idx_ 	= idx0_;
	}

	virtual SizeT get(T* buffer, SizeT length) const
	{
		const T* syms = sequence_->valuesBlock();

		SizeT idx = idx_;
		for (SizeT c = 0; c < length; c++, idx += BITSIZE)
		{
			buffer[c] = GetBits(syms, idx, BITSIZE);
		}

		return length;
	}

	virtual SizeT bits() const
	{
		return bits_;
	}

	virtual void getPrefix(T* buffer, SizeT start, SizeT length)
	{
		getSymbols(buffer, start, length);
		prefix_ = length;

		length_ = (bits_ - prefix_) / BITSIZE;
	}

	virtual void getSuffix(T* buffer)
	{
		getSymbols(buffer, 0, getSuffixSize());
	}

	virtual SizeT getSuffixSize() const
	{
		SizeT mask = TypeBitmask<T>();
		return (bits_ - prefix_) & mask;
	}

private:
	void getSymbols(T* buffer, SizeT start, SizeT length)
	{
		const T* syms = sequence_->valuesBlock();

		SetBits0(buffer, start, GetBits(syms, idx_, length), length);

		idx0_	+= length;
		idx_  	= idx0_;
	}
};





template <Int BitsPerSymbol, typename ItemType = UBigInt, typename IndexKeyType = UInt>
class SymbolSequence {

	typedef SymbolSequence<BitsPerSymbol> 										MyType;
	typedef PackedSeqTypes<IndexKeyType, ItemType, BitsPerSymbol>				Types;
	typedef PackedSeq<Types>													Seq;

	Seq* sequence_;

public:

	typedef typename Seq::Value													Symbol;


	static const Int Bits														= Seq::Bits;
	static const Int Symbols													= Seq::Symbols;

	typedef ISequenceDataSource<Symbol, Bits>									IDataSrc;
	typedef SequenceDataSourceAdapter<Seq>										SourceAdapter;

	SymbolSequence()
	{
		size_t size = 512;
		sequence_ 	= T2T<Seq*>(malloc(size));

		sequence_->initByBlock(size - sizeof(Seq));
	}

	~SymbolSequence()
	{
		free(T2T<void*>(sequence_));
	}

	SymbolSequence(size_t capacity)
	{
		Seq seq;
		seq.initSizes(capacity);
		size_t size = seq.getObjectSize();
		sequence_ = T2T<Seq*>(malloc(size));

		CopyByteBuffer(&seq, sequence_, sizeof(Seq));
	}

	SymbolSequence(const MyType& other)
	{
		size_t other_size = other.getObjectSize();
		sequence_ = T2T<Seq*>(malloc(other_size));
		CopyByteBuffer(other.sequence_, sequence_, other_size);
	}

	SymbolSequence(MyType&& other)
	{
		sequence_ = other.sequence_;
		other.sequence_ = nullptr;
	}

	size_t capacity() const
	{
		return sequence_->capacity();
	};

	size_t size() const
	{
		return sequence_->size();
	}

	size_t maxSize() const
	{
		return sequence_->maxSize();
	}

	void reindex()
	{
		sequence_->reindex();
	}

	void dump(ostream& out = cout) const
	{
		sequence_->dump(out);
	}

	void pushBack(Symbol symbol)
	{
		size_t size = sequence_->size();

		ensureCapacity(1);

		sequence_->value(size) = symbol;

		sequence_->size() = size + 1;
	}

	void ensureCapacity(size_t value)
	{
		if ((size_t)sequence_->capacity() < value)
		{
			Seq seq;
			seq.initSizes(sequence_->capacity() * 4 / 3);
			Seq* new_sequence = T2T<Seq*>(seq.getObjectSize());
			CopyByteBuffer(&seq, new_sequence, sizeof(Seq));

			free(T2T<void*>(sequence_));

			sequence_ = new_sequence;
		}
	}

private:

    class ValueSetter {
    	MyType&	me_;
    	size_t 	idx_;

    public:
    	ValueSetter(MyType& me, size_t idx):
    		me_(me),
    		idx_(idx)
    	{}

    	operator Symbol() const
    	{
    		return me_.sequence_->getValueItem(me_.sequence_->getValueBlockOffset(), idx_);
    	}

    	Symbol value() const
    	{
    		return operator Symbol();
    	}

    	Symbol operator=(const Symbol& v)
    	{
    		me_.sequence_->setValueItem(me_.sequence_->getValueBlockOffset(), idx_, v);
    		return v;
    	}
    };

public:

    ValueSetter operator[](size_t idx)
    {
    	return ValueSetter(*this, idx);
    }

    Symbol operator[](size_t idx) const
    {
    	return sequence_->getValueItem(sequence_->getValueBlockOffset(), idx);
    }

    SourceAdapter source(size_t idx, size_t length) const
    {
    	return SourceAdapter(sequence_, idx * BitsPerSymbol, length * BitsPerSymbol);
    }

    void update(size_t idx, IDataSrc& src)
    {
    	size_t bit_idx  = idx * Bits;

    	size_t bitsize	= TypeBitsize<Symbol>();
    	size_t mask 	= TypeBitmask<Symbol>();
    	size_t divisor 	= TypeBitmaskPopCount(mask);

    	size_t padding 	= bit_idx & mask;
    	size_t prefix  	= bitsize - padding;
    	size_t length 	= src.bits();

    	size_t pos 		= bit_idx >> divisor;
    	size_t bit_pos	= bit_idx & mask;

    	Symbol* symbols = sequence_->valuesBlock() + pos;

    	if (prefix >= length)
    	{
    		src.getPrefix(symbols, bit_pos, length);
    	}
    	else {
    		src.getPrefix(symbols, bit_pos, prefix);

    		symbols++;

    		while (src.getRemainder() > 0)
    		{
    			size_t size = src.get(symbols, src.getRemainder());
    			src.skip(size);
    			symbols += size;
    		}

    		if (src.getSuffixSize() > 0)
    		{
    			src.getSuffix(symbols);
    		}
    		else {
    			int a = 0; a++;
    		}
    	}
    }

    void fillCells(std::function<void(Symbol&)>&& f)
    {
    	size_t cells 	= sequence_->getTotalValueCells();
    	Symbol* symbols = sequence_->valuesBlock();

    	for (size_t c = 0; c < cells; c++)
    	{
    		f(symbols[c]);
    	}
    }

    void resize(size_t size)
    {
    	if (size > (size_t)sequence_->size())
    	{
    		ensureCapacity(size - sequence_->size());
    	}

    	sequence_->size() = size;
    }
};

}
}

#endif
