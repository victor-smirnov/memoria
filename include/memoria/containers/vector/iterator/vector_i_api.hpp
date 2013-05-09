
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_VECTOR_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_VECTOR_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector/vector_names.hpp>
#include <memoria/containers/vector/vector_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {

MEMORIA_ITERATOR_PART_BEGIN(memoria::mvector::ItrApiName)

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

		if (ctr.getNextNode(self.path()))
		{
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

	Int size() const
	{
		return self().leafSize(0);
	}

	MEMORIA_DECLARE_NODE_FN(ReadFn, read);


	BigInt read(DataTarget& data)
	{
		auto& self = this->self();
		mvector::VectorTarget target(&data);

		return self.model().readStream(self, target);
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
	BigInt skip(BigInt amount);


	struct PosFn {
		BigInt prefix_ = 0;

		template <typename NodeTypes, bool root, bool leaf>
		void treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int idx) {}

		template <typename NodeTypes, bool root, bool leaf>
		void treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, Int idx)
		{
			node->sum(0, 0, 0, idx, prefix_);
		}
	};


	BigInt pos() const
	{
		auto& self = this->self();

		PosFn fn;

		self.model().walkUp(self.path(), self.key_idx(), fn);

		return fn.prefix_ + self.key_idx();
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
		accum = prefix();
	}

	void ComputePrefix(Accumulator& accum)
	{
		accum = prefixes();
	}

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mvector::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS

M_PARAMS
BigInt M_TYPE::skip(BigInt amount)
{
    auto& self = this->self();

	if (amount > 0)
    {
        return self.skipFw(amount);
    }
    else if (amount < 0) {
        return self.skipBw(-amount);
    }
    else {
    	return 0;
    }
}


M_PARAMS
BigInt M_TYPE::skipFw(BigInt amount)
{
	return self().template _findFw<Types::template SkipForwardWalker>(0, amount);
}

M_PARAMS
BigInt M_TYPE::skipBw(BigInt amount)
{
	return self().template _findBw<Types::template SkipBackwardWalker>(0, amount);
}




}

#undef M_TYPE
#undef M_PARAMS

#endif
