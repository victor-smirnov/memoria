
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_ALLOCATOR_HPP
#define _MEMORIA_ALLOCATORS_PERSISTENT_INMEM_ALLOCATOR_HPP

#include <unordered_map>
#include <string>

#include <memoria/core/tools/stream.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <memoria/allocators/persistent-inmem/persistent_tree_node.hpp>
#include <memoria/allocators/persistent-inmem/persistent_tree_txn.hpp>
#include <memoria/allocators/persistent-inmem/persistent_tree.hpp>

#include <memoria/allocators/inmem/factory.hpp>

#include <malloc.h>
#include <memory>
#include <limits>

namespace memoria {

namespace detail = memoria::persistent_inmem;

using namespace std;

using namespace memoria::vapi;

template <typename Profile, typename PageType>
class PersistentInMemAllocatorT {
public:

	static constexpr Int NodeIndexSize 	= 32;
	static constexpr Int NodeSize 		= NodeIndexSize * 8;

	using MyType 		= PersistentInMemAllocatorT<Profile, PageType>;

	using Page 			= PageType;


	using Key 			= typename PageType::ID;
	using Value 		= PageType*;

	using TxnId 		 = UUID;
	using PTreeNodeId 	 = UUID;

	class PersistentTreeValue {
		Value page_;
		TxnId txn_id_;
	public:
		PersistentTreeValue() {}
		PersistentTreeValue(const Value& page, const TxnId& txn_id): page_(page), txn_id_(txn_id) {}

		const Value& page() const {return page_;}
		const TxnId& txn_id() const {return txn_id_;}
	};

	using LeafNodeT 	= memoria::persistent_inmem::LeafNode<Key, PersistentTreeValue, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
	using BranchNodeT 	= memoria::persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
	using NodeBaseT 	= typename BranchNodeT::NodeBaseT;
	using NodeBasePtr   = NodeBaseT*;

	struct HistoryNode;

	using HistoryNodePtr = HistoryNode*;


	struct HistoryNode {

		enum class Status {ACTIVE, COMMITTED};

	private:
		HistoryNodePtr parent_;
		std::vector<HistoryNodePtr> children_;

		NodeBasePtr root_;

		Status status_;

		TxnId txn_id_;

	public:

		HistoryNode(HistoryNodePtr parent, Status status = Status::ACTIVE):
			parent_(parent),
			root_(nullptr),
			status_(status),
			txn_id_(UUID::make_random())
		{
			if (parent_) {
				parent_->children().push_back(this);
			}
		}

		void remove_child(NodeBasePtr child)
		{
			for (size_t c = 0; c < children_.size(); c++)
			{
				if (children_[c] == child)
				{
					children_.erase(children_.begin() + c);
					break;
				}
			}
		}

		bool is_committed() const {
			return status_ == Status::COMMITTED;
		}

		bool is_active() const {
			return status_ == Status::ACTIVE;
		}

		const TxnId& txn_id() const {
			return txn_id_;
		}

		const NodeBasePtr root() const {
			return root_;
		}

		NodeBasePtr root() {
			return root_;
		}

		void set_root(NodeBasePtr new_root) {
			root_ = new_root;
		}

		PTreeNodeId new_node_id() {
			return UUID::make_random();
		}

		const HistoryNodePtr& parent() const {return parent_;}
		HistoryNodePtr& parent() {return parent_;}

		std::vector<HistoryNodePtr>& children() {
			return children_;
		}

		const std::vector<HistoryNodePtr>& children() const {
			return children_;
		}


		Status& status() {return status_;}
		const Status& status() const {return status_;}

		void commit() {
			status_ = Status::COMMITTED;
		}
	};

	using PersistentTreeT 	= memoria::persistent_inmem::PersistentTree<BranchNodeT, LeafNodeT, HistoryNode>;
	using TransactionT 	 	= memoria::persistent_inmem::Transaction<Profile, PageType, HistoryNode, PersistentTreeT, MyType>;
	using TransactionPtr 	= std::shared_ptr<TransactionT>;

	using TxnMap		 	= std::unordered_map<TxnId, HistoryNodePtr, UUIDKeyHash, UUIDKeyEq>;

	template <typename, typename, typename, typename>
	friend class Transaction;

private:
	HistoryNodePtr history_tree_ = nullptr;
	HistoryNodePtr master_ = nullptr;

	TxnMap transaction_map_;

	BigInt id_counter_ = 1;

	ContainerMetadataRepository*  metadata_;

public:

	PersistentInMemAllocatorT(): metadata_(MetadataRepository<Profile>::getMetadata())
	{
		history_tree_ = new HistoryNode(nullptr, HistoryNode::Status::ACTIVE);

		auto leaf = new LeafNodeT(history_tree_->txn_id(), UUID::make_random());
		history_tree_->set_root(leaf);

		master_ = history_tree_;

		TransactionT txn(history_tree_, this);

		txn.commit();
	}

	auto newId()
	{
		return typename PageType::ID(id_counter_++);
	}

	TransactionPtr find(const TxnId& branch_id)
	{
		auto iter = transaction_map_.find(branch_id);
		if (iter != transaction_map_.end())
		{
			auto history_node = iter.second;
			return std::make_shared<TransactionT>(history_node, this);
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Transaction id "<<branch_id<<" is not known");
		}
	}

	TransactionPtr master()
	{
		HistoryNodePtr node = new HistoryNode(master_);
		return std::make_shared<TransactionT>(node, this);
	}

	virtual void walkContainers(vapi::ContainerWalker* walker, const char* allocator_descr = nullptr)
	{
		walker->beginAllocator("PersistentInMemAllocator", allocator_descr);
		walk_version_tree(history_tree_, [](auto* txn){

		});
		walker->endAllocator();
	}


	virtual void load(InputStreamHandler *input)
	{

	}

	virtual void store(OutputStreamHandler *output)
	{
		char signature[12] = "MEMORIA";
		for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

		output->write(&signature, 0, sizeof(signature));

		walk_version_tree(history_tree_, [](auto* txn){

		});

		output->close();
	}

	ContainerMetadataRepository* getMetadata() const {
		return metadata_;
	}


private:

	void walk_version_tree(const HistoryNode* node, std::function<void (TransactionT*)> fn)
	{
		TransactionT txn(history_tree_, this);
		fn(&txn);

		for (auto child: node->children())
		{
			if (child->is_committed())
			{
				walk_version_tree(child, fn);
			}
		}
	}
};


template <typename Profile = DefaultProfile<>>
using PersistentInMemAllocator = class PersistentInMemAllocatorT<
		Profile,
		typename ContainerCollectionCfg<Profile>::Types::Page
>;



}



#endif
