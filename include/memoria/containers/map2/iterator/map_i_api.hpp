
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP2_ITERATOR_API_HPP
#define _MEMORIA_MODELS_IDX_MAP2_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/map2/map_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::map2::ItrApiName)

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

//	MyType& operator++(int) {
//		self().iter()++;
//		return self();
//	}
//
//	MyType& operator--(int) {
//		self().iter()--;
//		return self();
//	}

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


//	const TreePath& path() const
//	{
//		return self().iter().path();
//	}
//
//	TreePath& path()
//	{
//		return self().iter().path();
//	}

	void remove()
	{
		Accumulator keys;
		self().model().removeEntry(self(), keys);
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
