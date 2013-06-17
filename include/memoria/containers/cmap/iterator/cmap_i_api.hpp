
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_CMAP_ITER_API_HPP
#define _MEMORIA_CONTAINERS_CMAP_ITER_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/cmap/cmap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::cmap::ItrApiName)

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
		auto& self = this->self();

		self.model().updateUp(self.leaf(), self.idx(), keys, [&](Int level, Int idx) {
			if (level == 0)
			{
				self.idx() = idx;
			}
		});
	}

	Key rawKey() const
	{
		auto& self = this->self();
		return self.model().getLeafKey(self.leaf(), self.idx());
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
		return self().idx();
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


	void remove()
	{
		Accumulator keys;
		self().model().removeEntry1(self(), keys);
	}



    void ComputePrefix(BigInt& accum)
    {
    	NodeBaseG   node  = self().leaf();
    	Int         idx   = self().idx();

    	Accumulator acc0;

    	self().ctr().sumLeafKeys(node, 0, idx, acc0);

    	accum += std::get<0>(acc0)[0];

    	while (!node->is_root())
    	{
    		Int parent_idx 	= node->parent_idx();
    		node 			= self().ctr().getNodeParent(node);

    		self().ctr().sumKeys(node, 0, 0, parent_idx, accum);
    	}

    }

    void ComputePrefix(Accumulator& accum)
    {
    	NodeBaseG   node  = self().leaf();
    	Int         idx   = self().idx();

    	self().ctr().sumLeafKeys(node, 0, idx, accum);

    	while (!node->is_root())
    	{
    		Int parent_idx 	= node->parent_idx();
    		node 			= self().ctr().getNodeParent(node);

    		self().ctr().sumKeys(node, 0, parent_idx, accum);
    	}
    }



    void dump(std::ostream& out = std::cout) const {
    	auto cache = self().cache();

    	out<<"Cache:"
    		<<" prefix=" <<cache.prefix()
        	<<" current="<<cache.current()
        	<<endl;

    	Base::dump(out);
    }

MEMORIA_ITERATOR_PART_END

}

#endif
