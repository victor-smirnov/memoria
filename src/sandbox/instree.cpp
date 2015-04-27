
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/types/types.hpp>


#include <iostream>
#include <vector>
#include <list>
#include <memory>

#include <cstdlib>

using namespace std;
using namespace memoria;

class TreeNode {
public:
	virtual ~TreeNode() {}

	virtual void dump(Int level = 0) = 0;
};

template <typename Value>
class Node: public TreeNode {
protected:
	vector<Value> values_;
public:
	virtual ~Node() {}

	Int size() const {
		return values_.size();
	}

	Value& value(Int idx) {
		return values_[idx];
	}

	const Value& value(Int idx) const {
		return values_[idx];
	}

	void insert(const Value& val, Int at)
	{
		values_.insert(values_.begin() + at, val);
	}

	void append(const Value& val) {
		values_.push_back(val);
	}

	void remove(Int from, Int to)
	{
		values_.erase(values_.begin() + from, values_.begin() + to);
	}
};


using TreeNodePtr = shared_ptr<TreeNode>;

class BranchNode: public Node<TreeNodePtr> {

	using Base = Node<TreeNodePtr>;

public:
	virtual ~BranchNode() {
//		cout<<"BranchNode Destructed"<<endl;
	}

	virtual void dump(Int level)
	{
		for (Int c = 0; c < level * 4; c++) cout<<" ";
		cout<<"Branch of "<<Base::values_.size()<<" {"<<endl;
		for (auto& e: Base::values_)
		{
			e->dump(level + 1);
		}
		for (Int c = 0; c < level * 4; c++) cout<<" ";
		cout<<"}"<<endl;
	}
};



class LeafNode: public Node<Int> {
	using Base = Node<Int>;

public:
	virtual ~LeafNode() {
//		cout<<"LeafNode Destructed"<<endl;
	}


	virtual void dump(Int level)
	{
		for (Int c = 0; c < level*4; c++) cout<<" ";

		cout<<"Leaf of "<<Base::values_.size()<<" [";
		for (auto e: Base::values_)
		{
			cout<<e<<", ";
		}
		cout<<"]"<<endl;
	}
};




using BranchNodePtr = shared_ptr<BranchNode>;
using LeafNodePtr = shared_ptr<LeafNode>;

class Subtree {
	TreeNodePtr node_;
	Int subtree_size_;

public:
	Subtree(TreeNodePtr node, Int subtree_size): node_(node), subtree_size_(subtree_size) {}
	Subtree(): subtree_size_(0) {}

	TreeNodePtr node() {
		return node_;
	}

	const TreeNodePtr node() const {
		return node_;
	}

	Int subtree_size() const {
		return subtree_size_;
	}
};


class Checkpoint {
	using Iterator = list<LeafNodePtr>::iterator;

	Iterator iterator_;
	Int journal_size_;
public:
	Checkpoint(Iterator iter, Int size): iterator_(iter), journal_size_(size) {}

	Iterator iterator() const {return iterator_;};
	Int journal_size() const {return journal_size_;};
};


struct ILeafProvider {
	virtual LeafNodePtr get_leaf() 	= 0;

	virtual Checkpoint checkpoint() = 0;

	virtual void commit() 			= 0;
	virtual void rollback(const Checkpoint& chekpoint) = 0;

	virtual Int size() const		= 0;
};

Subtree BuildSubtree(ILeafProvider& leaf_provider, Int level)
{
	if (leaf_provider.size() > 0)
	{
//		cout<<"Provider Size: "<<leaf_provider.size()<<endl;

		if (level > 1)
		{
			BranchNodePtr node = make_shared<BranchNode>();

			Int max = 2 + rand() % 10;

			Int cnt = 0;

			for (Int c = 0; c < max && leaf_provider.size() > 0; c++)
			{
				Checkpoint checkpoint = leaf_provider.checkpoint();

				Subtree subtree = BuildSubtree(leaf_provider, level - 1);

				if (rand() % 5 != 0)
				{
					node->append(subtree.node());
					cnt += subtree.subtree_size();
				}
				else {
					leaf_provider.rollback(checkpoint);
				}
			}

			return Subtree(node, cnt);
		}
		else {
			return Subtree(leaf_provider.get_leaf(), 1);
		}
	}
	else {
		return Subtree();
	}
}


class ListLeafProvider: public ILeafProvider {
	list<LeafNodePtr> leafs_;
	list<LeafNodePtr>::iterator iterator_;

	Int journal_size_ = 0;
public:
	ListLeafProvider(): iterator_(leafs_.begin()) {}

	virtual Int size() const
	{
		return leafs_.size() - journal_size_;
	}

	virtual LeafNodePtr get_leaf()
	{
		if (iterator_ != leafs_.end())
		{
			auto item = *iterator_;
			iterator_++;
			journal_size_++;
			return item;
		}
		else {
			throw "Leaf List is empty";
		}
	}

	void add(LeafNodePtr leaf)
	{
		leafs_.push_back(leaf);

		journal_size_ = 0;
		iterator_ = leafs_.begin();
	}


	virtual Checkpoint checkpoint() {
		return Checkpoint(iterator_, journal_size_);
	}


	virtual void commit()
	{
		leafs_.erase(leafs_.begin(), iterator_);

		journal_size_ 	= 0;
		iterator_ 		= leafs_.begin();
	}

	virtual void rollback(const Checkpoint& checkpoint)
	{
		journal_size_ 	= checkpoint.journal_size();
		iterator_ 		= checkpoint.iterator();
	}
};

LeafNodePtr make_random_leaf(Int size)
{
	LeafNodePtr leaf = make_shared<LeafNode>();

	for (Int c = 0; c < size; c++)
	{
		leaf->append(rand() %100);
	}

	return leaf;
}

int main()
{
	try {
		ListLeafProvider provider;

		for (Int c = 0; c < 100; c++)
		{
			provider.add(make_random_leaf(rand() % 10 + 1));
		}

		auto tree = BuildSubtree(provider, 5);

		cout<<"Subtree Size:  "<<tree.subtree_size()<<endl;
		cout<<"Provider Size: "<<provider.size()<<endl;

		tree.node()->dump(0);
	}
	catch (const char* msg)
	{
		cout<<"Exception: "<<msg<<endl;
	}
	return 0;
}

