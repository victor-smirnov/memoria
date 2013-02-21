
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LOUDS_TOOLS_HPP_
#define MEMORIA_CONTAINERS_LOUDS_TOOLS_HPP_

namespace memoria 	{
namespace louds 	{

//class SelectResult {
//	BigInt node_idx_;
//	BigInt node_rank_;
//	Int value_;
//	bool found_;
//
//public:
//	SelectResult(BigInt node_idx, BigInt node_rank, Int value, bool found):
//		node_idx_(node_idx),
//		node_rank_(node_rank),
//		value_(value),
//		found_(found)
//	{}
//
//	BigInt node_idx() const {
//		return node_idx_;
//	}
//
//	BigInt node_rank() const {
//		return node_rank_;
//	}
//
//	bool found() const {
//		return found_;
//	}
//
//	Int value() const {
//		return value_;
//	}
//};

class LoudsNode {
protected:
	BigInt node_;
	BigInt node_rank_;

public:
	LoudsNode(BigInt node, BigInt node_rank):
		node_(node),
		node_rank_(node_rank)
	{}

	LoudsNode(BigInt node):
		node_(node),
		node_rank_(0)
	{}

	BigInt node() const {return node_;}
	BigInt node_rank() const {return node_rank_;}
};


class LoudsNodeRange: public LoudsNode {
	BigInt count_;
public:
	LoudsNodeRange(BigInt node, BigInt node_rank, BigInt count):
		LoudsNode(node, node_rank),
		count_(count)
	{}

	LoudsNodeRange(const LoudsNode& start, BigInt count):
		LoudsNode(start.node(), start.node_rank()),
		count_(count)
	{}

	BigInt count() const {return count_;}

	BigInt first() const {
		return node();
	}

	BigInt last() const {
		return node() + count();
	}
};

template <typename TreeType>
class Select0Walker {

protected:
	typedef typename TreeType::IndexKey IndexKey;
	typedef typename TreeType::Value 	Value;

	IndexKey 		rank0_;
	IndexKey 		rank1_;
	IndexKey 		limit_;

	bool			found_;

	const IndexKey* indexes0_;
	const IndexKey* indexes1_;
	const Value* 	buffer_;

public:
	Select0Walker(const TreeType& me, IndexKey limit):
		rank0_(0),
		rank1_(0),
		limit_(limit),
		found_(false)
	{
		indexes0_ 	= me.indexes(0);
		indexes1_ 	= me.indexes(1);

		buffer_ 	= me.valuesBlock();
	}

	void prepareIndex() {}

	Int walkIndex(Int start, Int end, Int cell_size)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey block_rank = indexes0_[c];

			if (block_rank < limit_)
			{
				rank0_ += block_rank;
				rank1_ += indexes1_[c];

				limit_ -= block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}

	Int walkValues(Int start, Int end)
	{
		auto result = Select0FW(buffer_, start, end, limit_);

		rank1_  += PopCount(buffer_, start, result.idx() + 1);

		rank0_  += result.rank();
		limit_  -= result.rank();

		found_  = result.is_found() || limit_ == 0;

		return result.idx();
	}


	IndexKey rank0() const
	{
		return rank0_;
	}

	IndexKey rank1() const
	{
		return rank1_;
	}

	bool is_found() const
	{
		return found_;
	}
};


template <typename T>
class LoudsSourceAdaptorBase: public ISequenceDataSource<T, 1> {
protected:
    SizeT       start_;
    SizeT       length_;

public:

    LoudsSourceAdaptorBase(BigInt length):
        start_(0),
        length_(length)
    {}



    virtual ~LoudsSourceAdaptorBase() throw () {}

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

    SizeT size() const {
        return getSize();
    }

    virtual void reset()
    {
        start_ = 0;
    }
};





template <typename T>
class NodeDegreeSourceAdaptor: public LoudsSourceAdaptorBase<T> {

	typedef LoudsSourceAdaptorBase<T> Base;

public:

    NodeDegreeSourceAdaptor(BigInt degree):
        Base(degree + 1)
    {}

    virtual ~NodeDegreeSourceAdaptor() throw () {}

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
    	FillOne(buffer, start, start + length);

        SizeT result = Base::skip(length);

        if (Base::getRemainder() == 0)
        {
        	SetBit(buffer, start + length - 1, 0);
        }

        return result;
    }
};


template <typename T>
class ZeroSourceAdaptor: public LoudsSourceAdaptorBase<T> {

	typedef LoudsSourceAdaptorBase<T> Base;

public:

    ZeroSourceAdaptor(BigInt length):
        Base(length)
    {}

    virtual ~ZeroSourceAdaptor() throw () {}

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
    	FillZero(buffer, start, start + length);
        return Base::skip(length);
    }
};




}
}


#endif /* LOUDS_TOOLS_HPP_ */
