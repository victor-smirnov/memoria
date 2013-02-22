
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

template <typename Seq>
class SequenceDataSourceAdapter: public ISequenceDataSource<typename Seq::Symbol, Seq::Bits> {

	static const SizeT Bits		= Seq::Bits;

	typedef typename Seq::Symbol T;

	SizeT   start0_;
	SizeT   start_;
	SizeT   length_;

	const Seq*	sequence_;
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

	virtual void  reset()
	{
		start_ 	= start0_;
	}

	virtual SizeT get(T* buffer, SizeT start, SizeT length)
	{
		const T* syms = sequence_->valuesBlock();

		MoveBits(syms, buffer, start_ * Bits, start * Bits, length * Bits);

		return skip(length);
	}
};



template <typename Seq>
class SequenceDataTargetAdapter: public ISequenceDataTarget<typename Seq::Symbol, Seq::Bits> {

	static const SizeT Bits		= Seq::Bits;

	typedef typename Seq::Symbol T;

	SizeT   start0_;
	SizeT   start_;
	SizeT   length_;

	Seq*	sequence_;
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

	virtual void  reset()
	{
		start_ 	= start0_;
	}

	virtual SizeT put(const T* buffer, SizeT start, SizeT length)
	{
		T* syms = sequence_->valuesBlock();

		MoveBits(buffer, syms, start * Bits, start_ * Bits, length * Bits);

		return skip(length);
	}
};




template <Int BitsPerSymbol, typename ItemType = UBigInt, typename IndexKeyType = UInt>
class SymbolSequence {
protected:
	typedef SymbolSequence<BitsPerSymbol> 										MyType;
	typedef PackedSeqTypes<IndexKeyType, ItemType, BitsPerSymbol>				Types;
	typedef PackedSeq<Types>													Seq;

	Seq* sequence_;

public:

	typedef typename Seq::Value													Symbol;


	static const Int Bits														= Seq::Bits;
	static const Int Symbols													= Seq::Symbols;

	typedef ISequenceDataSource<Symbol, Bits>									IDataSrc;
	typedef ISequenceDataTarget<Symbol, Bits>									IDataTgt;
	typedef SequenceDataSourceAdapter<Seq>										SourceAdapter;
	typedef SequenceDataTargetAdapter<Seq>										TargetAdapter;

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
		size_t other_size = other.sequence_->getObjectSize();
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

	void enlarge(size_t size)
	{
		ensureCapacity(size);
		sequence_->size() += size;
	}

	bool ensureCapacity(size_t value)
	{
		if ((size_t)sequence_->capacity() < value)
		{
			Seq seq;
			seq.initSizes(sequence_->maxSize() * 4 / 3);
			seq.size() = sequence_->size();

			if ((size_t)seq.capacity() < value)
			{
				seq.initSizes(sequence_->maxSize() + value*2);
			}

			seq.size() = sequence_->size();

			size_t new_size = seq.getObjectSize();

			Seq* new_sequence = T2T<Seq*>(malloc(new_size));
			CopyByteBuffer(&seq, new_sequence, sizeof(Seq));

			sequence_->transferTo(new_sequence);

			free(T2T<void*>(sequence_));

			sequence_ = new_sequence;

			return true;
		}

		return false;
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
    	return SourceAdapter(sequence_, idx, length);
    }

    SourceAdapter source() const
    {
    	return source(0, sequence_->size());
    }

    TargetAdapter target(size_t idx, size_t length) const
    {
    	return TargetAdapter(sequence_, idx, length);
    }

    TargetAdapter target() const
    {
    	return target(0, sequence_->size());
    }

    void update(size_t start, IDataSrc& src)
    {
    	Symbol* syms = sequence_->valuesBlock();
    	src.get(syms, start, src.getSize());
    }

    void insert(size_t at, IDataSrc& src)
    {
    	sequence_->insertSpace(at, src.getSize());
    	update(at, src);
    }

    void append(IDataSrc& src)
    {
    	size_t at = sequence_->size();
    	ensureCapacity(src.getSize());
    	sequence_->insertSpace(at, src.getSize());
    	update(at, src);
    }


    void read(size_t from, IDataTgt& tgt) const
    {
    	const Symbol* syms = sequence_->valuesBlock();
    	tgt.put(syms, from, tgt.getSize());
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

#endif
