
// Copyright Victor Smirnov 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_WALKERS_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_WALKERS_HPP

#include <memoria/prototypes/sequence/names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/tools/idata.hpp>

namespace memoria    {



namespace sequence {


template <typename Types>
class FindWalkerBase {
protected:
	typedef typename Types::Key 												Key;
	typedef Iter<typename Types::IterTypes> 									Iterator;

	Key key_;
	Int key_num_;

	Key prefix_;
	Int idx_;
public:
	FindWalkerBase(Key key, Int key_num):
		key_(key), key_num_(key_num), prefix_(0), idx_(0)
	{}

	template <typename Node>
	Int checkIdxBounds(Int idx, const Node* node) const
	{
		if (idx < node->children_count())
		{
			return idx;
		}
		else
		{
			return node->children_count() - 1;
		}
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.init();
	}

	void empty(Iterator& iter)
	{
		iter.init();
	}

	Int idx() const {
		return idx_;
	}
};


template <typename Types>
class FindLTWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

public:
	FindLTWalker(Key key, Int key_num): Base(key, key_num)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = node->map().findLTS(Base::key_num_, Base::key_ - Base::prefix_, Base::prefix_);
	}
};

template <typename Types>
class FindLEWalker: public FindWalkerBase<Types> {

	typedef FindWalkerBase<Types> 		Base;
	typedef typename Base::Key 			Key;

public:
	FindLEWalker(Key key, Int key_num): Base(key, key_num)
	{}

	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = node->map().findLES(Base::key_num_, Base::key_ - Base::prefix_, Base::prefix_);
	}
};


template <typename Types>
class FindRangeWalkerBase {
protected:
	typedef Iter<typename Types::IterTypes> Iterator;
	typedef Ctr<typename Types::CtrTypes> 	Container;

	Int idx_;
public:
	FindRangeWalkerBase(): idx_(0) {}

	template <typename Node>
	Int checkIdxBounds(Int idx, const Node* node) const
	{
		return idx;
	}

	void empty(Iterator& iter)
	{
		iter.init();
	}

	Int idx() const
	{
		return idx_;
	}
};



template <typename Types>
class FindEndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

public:
	FindEndWalker(Container& ctr) {}

	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = node->children_count() - 1;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = iter.data()->size();

		iter.init();
	}
};


template <typename Types>
class FindREndWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

public:
	FindREndWalker(Container&) {}

	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = -1;

		iter.init();
	}
};




template <typename Types>
class FindBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

public:
	FindBeginWalker(Container&) {}


	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = 0;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = 0;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = 0;

		iter.init();
	}
};

template <typename Types>
class FindRBeginWalker: public FindRangeWalkerBase<Types> {

	typedef FindRangeWalkerBase<Types> 		Base;
	typedef typename Base::Iterator 		Iterator;
	typedef typename Base::Container 		Container;

public:
	FindRBeginWalker(Container&) {}

	template <typename Node>
	void operator()(const Node* node)
	{
		Base::idx_ = node->children_count() - 1;
	}

	void finish(Int idx, Iterator& iter)
	{
		iter.key_idx() = idx;

		iter.model().finishPathStep(iter.path(), iter.key_idx());
		iter.dataPos() = iter.data()->size() - 1;

		iter.init();
	}
};



}


template <typename Iterator>
class IDataAdapter: public IData<typename Iterator::ElementType> {
    typedef typename Iterator::ElementType T;

    Iterator    iter_;
    BigInt      length_;

    BigInt      start_  = 0;

public:
    IDataAdapter(IDataAdapter<Iterator>&& other):
        iter_(std::move(other.iter_)), length_ (other.length_), start_(other.start_) {}

    IDataAdapter(const IDataAdapter<Iterator>& other):
            iter_(other.iter_), length_ (other.length_), start_(other.start_) {}

    IDataAdapter(const Iterator& iter, BigInt length): iter_(iter)
    {
        BigInt pos  = iter_.pos();
        BigInt size = iter_.model().size();

        if (length != -1 && (pos + length <= size))
        {
            length_ = length;
        }
        else {
            length_ = size - pos;
        }
    }

    virtual SizeT skip(SizeT length)
    {
        BigInt delta = iter_.skip(length);
        start_ += delta;
        return delta;
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

    virtual SizeT put(const T* buffer, SizeT start, SizeT length)
    {
        return 0;
    }

    virtual SizeT get(T* buffer, SizeT start, SizeT length)
    {
        auto& data  = iter_.data();
        Int pos     = iter_.dataPos();
        BigInt size = data->getMaxCapacity() - pos;

        if (length > size)
        {
            length = size;
        }

        CopyBuffer(data->values() + pos, buffer + start, length);

        return skip(length);
    }

    virtual void reset()
    {
        iter_.skip(-start_);
        start_ = 0;
    }
};

}

#endif
