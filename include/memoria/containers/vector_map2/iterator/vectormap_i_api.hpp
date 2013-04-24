
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_VECTORMAP2_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_VECTORMAP2_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector2/vector_names.hpp>
#include <memoria/containers/vector2/vector_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {




MEMORIA_ITERATOR_PART_BEGIN(memoria::vmap::ItrApiName)

	typedef Ctr<typename Types::CtrTypes>                      	Container;


	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Container::Value                                     		Value;
	typedef typename Container::Key                                       		Key;
	typedef typename Container::Element                                   		Element;
	typedef typename Container::Accumulator                               		Accumulator;

	typedef typename Container::DataSource                                		DataSource;
	typedef typename Container::DataTarget                                		DataTarget;
	typedef typename Container::LeafDispatcher                                	LeafDispatcher;
	typedef typename Container::Position										Position;

	bool operator++() {
		return self().skipFw(1);
	}

	bool operator--() {
		return self().skipBw(1);
	}

	bool operator++(int) {
		return self().skipFw(1);
	}

	bool operator--(int) {
		return self().skipFw(1);
	}

	BigInt operator+=(BigInt size)
	{
		return self().skipFw(size);
	}

	BigInt operator-=(BigInt size)
	{
		return self().skipBw(size);
	}

	bool isEof() const {
		return self().key_idx() >= self().size();
	}

	bool isBof() const {
		return self().key_idx() < 0;
	}

	bool nextLeaf()
	{
		auto& self 		= this->self();
		auto& ctr 		= self.model();

		Int size 		= self.size();
		BigInt prefix 	= self.prefix();

		if (ctr.getNextNode(self.path()))
		{
			self.cache().setup(prefix + size);
			self.key_idx() = 0;
			return true;
		}
		else {
			return false;
		}
	}

	void insert(std::vector<Value>& data)
	{
		auto& self = this->self();
		auto& model = self.model();

		MemBuffer<Value> buf(data);

		model.insert(self, buf);
	}


	MEMORIA_DECLARE_NODE_FN_RTN(SizeFn, size, Int);

	Int size() const
	{
		return LeafDispatcher::dispatchConstRtn(self().leaf().node(), SizeFn(), 0);
	}

	MEMORIA_DECLARE_NODE_FN(ReadFn, read);


	BigInt read(DataTarget& data)
	{
		auto& self = this->self();

		BigInt sum = 0;
		BigInt len = data.getRemainder();

		while (len > 0)
		{
			Int to_read = self.size() - self.dataPos();

			if (to_read > len) to_read = len;

			mvector2::VectorTarget target(&data);

			LeafDispatcher::dispatchConst(self.leaf().node(), ReadFn(), &target, Position(self.dataPos()), Position(to_read));

			len     -= to_read;
			sum     += to_read;

			self.skipFw(to_read);

			if (self.isEof())
			{
				break;
			}
		}

		return sum;
	}

	BigInt read(std::vector<Value>& data)
	{
		MemTBuffer<Value> buf(data);
		return read(buf);
	}

	void remove(BigInt size)
	{
		auto& self = this->self();
		self.model().remove(self, size);
	}

	std::vector<Value> subVector(BigInt size)
	{
		std::vector<Value> data(size);

		auto iter = self();

		iter.read(data);

		return data;
	}

	BigInt skipFw(BigInt amount);
	BigInt skipBw(BigInt amount);

	BigInt pos() const {
		return prefix() + self().key_idx();
	}

	BigInt dataPos() const {
		return self().key_idx();
	}

	BigInt prefix() const {
		return self().cache().prefix();
	}

	Accumulator prefixes() const {
		Accumulator acc;
		std::get<0>(acc)[0] = prefix();
		return acc;
	}

	void ComputePrefix(BigInt& accum)
	{
		TreePath&   path0 = self().path();
		Int         idx   = self().key_idx();

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

		for (Int c = 1; c < path0.getSize(); c++)
		{
			idx = path0[c - 1].parent_idx();
			self().model().sumKeys(path0[c].node(), 0, idx, accum);
		}
	}

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::vmap::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::skipFw(BigInt amount)
{
	typedef vmap::FindLTForwardWalker<Types> Walker;

	auto& self = this->self();

	Walker walker(amount, 0, self.prefixes());

	BigInt pos = self.pos();

	Int idx = self.key_idx() = self.model().findFw(self.path(), 0, self.key_idx(), walker);

	Int last_size = self.size();

	if (idx >= last_size)
	{
		self.cache().setup(pos + walker.prefix() - last_size);
		return walker.prefix();
	}
	else if (walker.leafs() == 2)
	{
		self.cache().setup(pos + walker.prefix());
	}

	return walker.prefix() + idx;
}

M_PARAMS
BigInt M_TYPE::skipBw(BigInt amount)
{
	typedef vmap::FindLTBackwardWalker<Types> Walker;

	auto& self = this->self();

	BigInt pos = self.pos();

	Walker walker(amount, 0);

	Int idx = self.key_idx() = self.model().findBw(self.path(), 0, self.key_idx(), walker);

	if (idx >= 0)
	{
		self.cache().setup(pos - walker.prefix());
		return walker.prefix() - idx;
	}
	else {
		self.cache().setup(0);
		return walker.prefix();
	}
}


}

#undef M_TYPE
#undef M_PARAMS

#endif
