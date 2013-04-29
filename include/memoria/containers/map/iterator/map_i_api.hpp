
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAP_ITER_API_HPP
#define _MEMORIA_CONTAINERS_MAP_ITER_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::map::ItrApiName)

	typedef Ctr<typename Types::CtrTypes>                      	Container;


	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Container::Value                                     Value;
	typedef typename Container::Key                                       Key;
	typedef typename Container::Element                                   Element;
	typedef typename Container::Accumulator                               Accumulator;


	void updateUp(const Accumulator& keys)
	{
		self().model().updateUp(self().path(), 0, self().entry_idx(), keys);
	}

	Key rawKey() const {
		return self().model().getLeafKey(leaf(), entry_idx());
	}

	Key key() const {
		return self().prefix() + rawKey();
	}

	bool next() {
		return self().nextKey();
	}

	bool prev() {
		return self().prevKey();
	}

	BigInt prefix() const
	{
		return self().cache().prefix();
	}

	Accumulator prefixes() const {
		Accumulator acc;

		std::get<0>(acc)[0] = self().prefix();

		return acc;
	}

	Int entry_idx() const {
		return self().key_idx();
	}


	void setValue(const Value& value)
	{
		self().model().setLeafData(self().leaf(), self().entry_idx(), value);
	}


	class ValueAccessor {
		MyType& iter_;
	public:
		ValueAccessor(MyType& iter): iter_(iter) {}

		operator Value() const {
			return iter_.getValue();
		}

		Value operator=(const Value& value) {
			iter_.setValue(value);
			return value;
		}
	};

	class ConstValueAccessor {
		const MyType& iter_;
	public:
		ConstValueAccessor(const MyType& iter): iter_(iter) {}

		operator Value() const {
			return iter_.getValue();
		}
	};


	Value getValue() const {
		return self().model().getLeafData(self().leaf(), self().entry_idx());
	}

	ValueAccessor value() {
		return ValueAccessor(self());
	}

	ConstValueAccessor value() const {
		return ConstValueAccessor(self());
	}


	void setData(const Value& value) {
		self().value() = value;
	}

	const NodeBaseG& leaf() const
	{
		return self().path().leaf();
	}

	NodeBaseG& leaf()
	{
		return self().path().leaf();
	}


	void remove()
	{
		Accumulator keys;
		self().model().removeEntry1(self(), keys);
	}



    void ComputePrefix(BigInt& accum)
    {
    	TreePath&   path0 = self().path();
    	Int         idx   = self().key_idx();

    	Accumulator acc0;

    	self().model().sumLeafKeys(path0[0].node(), 0, idx, acc0);

    	accum += std::get<0>(acc0)[0];

    	for (Int c = 1; c < path0.getSize(); c++)
    	{
    		idx = path0[c - 1].parent_idx();
    		self().model().sumKeys(path0[c].node(), 0, 0, idx, accum);
    	}
    }

    void ComputePrefix(Accumulator& accum)
    {
    	TreePath&   path0 = self().path();
    	Int         idx   = self().key_idx();

    	self().model().sumLeafKeys(path0[0].node(), 0, idx, accum);

    	for (Int c = 1; c < path0.getSize(); c++)
    	{
    		idx = path0[c - 1].parent_idx();
    		self().model().sumKeys(path0[c].node(), 0, idx, accum);
    	}
    }


//
//	void dump(ostream& out = cout)
//	{
//		self().().dump(out);
//	}

MEMORIA_ITERATOR_PART_END

}

#endif
