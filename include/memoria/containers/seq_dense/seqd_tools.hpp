
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_TOOLS_HPP

#include <memoria/prototypes/bt/layouts/bt_input_buffer.hpp>
#include <memoria/core/tools/isymbols.hpp>

namespace memoria       {
namespace seq_dense     {


template <Int Bits>
class SymbolsInputBufferProvider: public bt::InputBufferProvider<Int, vapi::SymbolsBuffer<Bits>> {

	using Buffer = vapi::SymbolsBuffer<Bits>;

	using Position = Int;

	vapi::SymbolsBuffer<Bits>& data_;
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

	virtual Position size()	const {
		return size_;
	}

	virtual Position zero()	const {return 0;}

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


class SequenceSource: public ISource {

    IDataBase* source_;
public:
    SequenceSource(IDataBase* source): source_(source) {}

    virtual Int streams()
    {
        return 1;
    }

    virtual IData* stream(Int stream)
    {
        return source_;
    }

    virtual void newNode(INodeLayoutManager* layout_manager, BigInt* sizes)
    {
        Int allocated[1] = {0};
        Int capacity = layout_manager->getNodeCapacity(allocated, 0);

        sizes[0] = capacity;
    }

    virtual BigInt getTotalNodes(INodeLayoutManager* manager)
    {
        Int sizes[1] = {0};

        SizeT capacity  = manager->getNodeCapacity(sizes, 0);
        SizeT remainder = source_->getRemainder();

        return remainder / capacity + (remainder % capacity ? 1 : 0);
    }
};


class SequenceTarget: public ITarget {

    IDataBase* target_;
public:
    SequenceTarget(IDataBase* target): target_(target) {}

    virtual Int streams()
    {
        return 1;
    }

    virtual IData* stream(Int stream)
    {
        return target_;
    }
};



template <typename Iterator, typename Container>
class SequenceIteratorCache: public bt::BTreeIteratorCache<Iterator, Container> {

    typedef bt::BTreeIteratorCache<Iterator, Container>                         Base;

    BigInt pos_ = 0;

public:

    SequenceIteratorCache(): Base() {}

    BigInt pos() const
    {
        return pos_;
    }

    void setup(BigInt pos)
    {
        pos_    = pos;
    }

    void add(BigInt pos)
    {
        pos_    += pos;
    }

    void sub(BigInt pos)
    {
        pos_    -= pos;
    }
};



}
}

#endif
