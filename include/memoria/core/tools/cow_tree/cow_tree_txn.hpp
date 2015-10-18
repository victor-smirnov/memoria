
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef INCLUDE_MEMORIA_CORE_TOOLS_COW_TREE_TXN_HPP_
#define INCLUDE_MEMORIA_CORE_TOOLS_COW_TREE_TXN_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/cow_tree/cow_tree_sharedptr.hpp>

#include <memoria/core/tools/static_array.hpp>

#include <deque>
#include <mutex>
#include <iostream>

/**
 * Serializable Copy-on-write B+Tree. Transaction Log.
 */

namespace memoria 	{
namespace cow 		{
namespace tree 		{

template <typename NodeBase>
class CoWTreeTxnLogEntry {
	NodeBase* root_;
	BigInt txn_id_;

	std::mutex* lock_;

	Int refs_ = 0;

public:
	using NodeBaseT = NodeBase;

	CoWTreeTxnLogEntry(NodeBase* root, BigInt txn_id, std::mutex* lock):
		root_(root), txn_id_(txn_id), lock_(lock)
	{}

	NodeBase* root() {
		return root_;
	}

	const NodeBase* root() const {
		return root_;
	}

	BigInt txn_id() const {
		return txn_id_;
	}

	void ref()
	{
		// must be locked
		refs_++;
	}

	void unref()
	{
		// must be locked
		refs_--;
	}
};




template <typename LogEntry>
class Snapshot: public CoWSharedPtr<Snapshot<LogEntry>, LogEntry> {
	using Base = CoWSharedPtr<Snapshot<LogEntry>, LogEntry>;

	using NodeBaseT = typename LogEntry::NodeBaseT;

public:
	using MyType = Snapshot<LogEntry>;

	Snapshot(): Base() {}
	Snapshot(const MyType& other): Base(other) 			{}
	Snapshot(MyType&& other): Base(std::move(other)) 	{}

	Snapshot(LogEntry& entry): Base(&entry) {}

	~Snapshot() {}

	BigInt txn_id() const  {
		return root()->txn_id();
	}

	NodeBaseT* root() {
		return this->get()->root();
	}

	const NodeBaseT* root() const {
		return this->get()->root();
	}
};


template <typename TxnData>
class Transaction: public CoWSharedPtr<Transaction<TxnData>, TxnData> {
	using Base = CoWSharedPtr<Transaction<TxnData>, TxnData>;
public:
	using NodeBaseT = typename TxnData::NodeBaseT;


public:
	using MyType = Transaction<TxnData>;

	Transaction(): Base() {}
	Transaction(const MyType& other): Base(other) 		{}
	Transaction(MyType&& other): Base(std::move(other)) {}

	Transaction(TxnData& data): Base(&data) {}

	~Transaction() 	{}

	MyType new_tread() = delete;

	void commit() {
		this->get()->commit();
	}
	void rollback() {
		this->get()->rollback();
	}

	BigInt txn_id() const  {
		return root()->txn_id();
	}

	NodeBaseT* root() {
		return this->get()->root();
	}

	const NodeBaseT* root() const {
		return this->get()->root();
	}
};




template <typename NodeBase>
class TxnLog {
public:
	using LogEntryT 	= CoWTreeTxnLogEntry<NodeBase>;

	using SnapshotT 	= Snapshot<LogEntryT>;

protected:
	std::mutex lock_;

	std::deque<LogEntryT> events_;

public:

	TxnLog() {}

	std::mutex& lock() {
		return lock_;
	}

	void new_entry(NodeBase* root)
	{
		LogEntryT entry(root, root->txn_id(), &lock_);
		events_.push_front(entry);
	}

	auto size() const {
		return events_.size();
	}

	auto& front() {
		return events_.front();
	}

	const auto& front() const {
		return events_.front();
	}

	SnapshotT snapshot()
	{
		return SnapshotT(events_.front());
	}

	SnapshotT snapshot(BigInt txn_id)
	{
		LogEntryT* entry = nullptr;

		for (LogEntryT& e: events_)
		{
			if (e.txn_id() == txn_id)
			{
				entry = &e;
			}
		}

		return SnapshotT(entry);
	}

	void dump(std::ostream& out = std::cout) const
	{
		out<<"TxnLog of "<<events_.size()<<" entries."<<endl;

		Int c = 0;
		for (const auto& entry: events_)
		{
			out<<c
			   <<": TxnId: "<<entry.txn_id()
			   <<", NodeId: "<<entry.root()->node_id()
			   <<", size: "<<entry.root()->metadata().size()
			   <<endl;
		}
	}
};


}
}
}

#endif
