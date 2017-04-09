
// Copyright 2016 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/latch.hpp>

#include "persistent_tree_node.hpp"
#include "persistent_tree.hpp"
#include "persistent_tree_snapshot.hpp"


#include <malloc.h>
#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>



namespace memoria {
namespace v1 {

namespace details {

	template <typename PageT>
	struct PagePtr {
		using RefCntT = BigInt;
	private:
		PageT* page_;
		std::atomic<RefCntT> refs_;

		// Currently for debug purposes
		static std::atomic<BigInt> page_cnt_;
	public:
		PagePtr(PageT* page, BigInt refs): page_(page), refs_(refs) {
			//page_cnt_++;
			//cout << "Create page: " << page_cnt_ << endl;
		}

		~PagePtr() {
			//if (--page_cnt_ == 0) {
			//	cout << "All Pages removed" << endl;
			//}
			//cout << "Remove page: " << page_cnt_ << endl;

			::free(page_);
		}

		PageT* raw_data() {return page_;}
		const PageT* raw_data() const {return page_;}

		PageT* operator->() {return page_;}
		const PageT* operator->() const {return page_;}

		void clear() {
			page_ = nullptr;
		}

		void ref() {
			refs_++;
		}

		RefCntT references() const {return refs_;}

		RefCntT unref() {
			return --refs_;
		}
	};

	template <typename PageT>
	std::atomic<BigInt> PagePtr<PageT>::page_cnt_(0);


	template <typename ValueT, typename TxnIdT>
	class PersistentTreeValue {
		ValueT page_;
		TxnIdT txn_id_;
	public:
		using Value = ValueT;

		PersistentTreeValue(): page_(), txn_id_() {}
		PersistentTreeValue(const ValueT& page, const TxnIdT& txn_id): page_(page), txn_id_(txn_id) {}

		const ValueT& page_ptr() const {return page_;}
		ValueT& page_ptr() {return page_;}

		const TxnIdT& txn_id() const {return txn_id_;}
		TxnIdT& txn_id() {return txn_id_;}
	};

}


template <typename V, typename T>
OutputStreamHandler& operator<<(OutputStreamHandler& out, const v1::details::PersistentTreeValue<V, T>& value)
{
    out << value.page_ptr();
    out << value.txn_id();
    return out;
}

template <typename V, typename T>
InputStreamHandler& operator>>(InputStreamHandler& in, v1::details::PersistentTreeValue<V, T>& value)
{
    in >> value.page_ptr();
    in >> value.txn_id();
    return in;
}

template <typename V, typename T>
std::ostream& operator<<(std::ostream& out, const v1::details::PersistentTreeValue<V, T>& value)
{
    out<<"PersistentTreeValue[";
    out << value.page_ptr()->raw_data();
    out<<", ";
    out << value.page_ptr()->references();
    out<<", ";
    out << value.txn_id();
    out<<"]";

    return out;
}


template <typename Profile, typename PageType>
class PersistentInMemAllocatorT: public std::enable_shared_from_this<PersistentInMemAllocatorT<Profile, PageType>> {
public:

    static constexpr Int NodeIndexSize  = 32;
    static constexpr Int NodeSize       = NodeIndexSize * 32;

    using MyType        = PersistentInMemAllocatorT<Profile, PageType>;


    template <typename T>
    using CtrSharedPtr = memoria::v1::CtrSharedPtr<Profile, T>;
    
    template <typename T>
    using CtrMakeSharedPtr = memoria::v1::CtrMakeSharedPtr<Profile, T>;
    
    using Page          = PageType;
    using RCPagePtr		= v1::details::PagePtr<Page>;

    using Key           = typename PageType::ID;
    using Value         = PageType*;

    using TxnId             = UUID;
    using PTreeNodeId       = UUID;

    using LeafNodeT         = v1::persistent_inmem::LeafNode<Key, v1::details::PersistentTreeValue<RCPagePtr*, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
    using LeafNodeBufferT   = v1::persistent_inmem::LeafNode<Key, v1::details::PersistentTreeValue<typename PageType::ID, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;

    using BranchNodeT       = v1::persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
    using BranchNodeBufferT = v1::persistent_inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId, PTreeNodeId>;
    using NodeBaseT         = typename BranchNodeT::NodeBaseT;
    using NodeBaseBufferT   = typename BranchNodeBufferT::NodeBaseT;

    using MutexT			= std::recursive_mutex;
    using SnapshotMutexT	= std::recursive_mutex;
    using StoreMutexT		= std::mutex;

    using LockGuardT			= std::lock_guard<MutexT>;
    using StoreLockGuardT		= std::lock_guard<StoreMutexT>;
    using SnapshotLockGuardT	= std::lock_guard<SnapshotMutexT>;

    struct HistoryNode {

    	using Status 		= persistent_inmem::SnapshotStatus;
        using HMutexT 		= SnapshotMutexT;
        using HLockGuardT 	= SnapshotLockGuardT;

    private:
        MyType* allocator_;

        HistoryNode* parent_;
        String metadata_;

        std::vector<HistoryNode*> children_;

        NodeBaseT* root_;

        typename PageType::ID root_id_;

        Status status_;

        TxnId txn_id_;

        BigInt references_ = 0;

        mutable HMutexT mutex_;

    public:

        HistoryNode(MyType* allocator, Status status = Status::ACTIVE):
        	allocator_(allocator),
            parent_(nullptr),
            root_(nullptr),
            root_id_(),
            status_(status),
            txn_id_(UUID::make_random())
        {
            if (parent_) {
                parent_->children().push_back(this);
            }
        }

        HistoryNode(HistoryNode* parent, Status status = Status::ACTIVE):
        	allocator_(parent->allocator_),
			parent_(parent),
			root_(nullptr),
			root_id_(),
			status_(status),
			txn_id_(UUID::make_random())
        {
        	if (parent_) {
        		parent_->children().push_back(this);
        	}
        }

        HistoryNode(MyType* allocator, const TxnId& txn_id, HistoryNode* parent, Status status):
        	allocator_(allocator),
            parent_(parent),
            root_(nullptr),
            root_id_(),
            status_(status),
            txn_id_(txn_id)
        {
            if (parent_) {
                parent_->children().push_back(this);
            }
        }

        auto* allocator() {
        	return allocator_;
        }

        const auto* callocator() const {
        	return allocator_;
        }

        auto& allocator_mutex() {return allocator_->mutex_;}
        const auto& allocator_mutex() const {return allocator_->mutex_;}

        auto& store_mutex() {return allocator_->store_mutex_;}
        const auto& store_mutex() const {return allocator_->store_mutex_;}

        auto& snapshot_mutex() {return mutex_;}
        const auto& snapshot_mutex() const {return mutex_;}

        void remove_child(HistoryNode* child)
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

        void remove_from_parent() {
            if (parent_) {
                parent_->remove_child(this);
            }
        }

        bool is_committed() const
        {
            return status_ == Status::COMMITTED;
        }

        bool is_active() const
        {
            return status_ == Status::ACTIVE;
        }

        bool is_dropped() const
        {
            return status_ == Status::DROPPED;
        }

        bool is_data_locked() const
        {
        	return status_ == Status::DATA_LOCKED;
        }

        const TxnId& txn_id() const {return txn_id_;}

        const auto& root() const {
        	return root_;
        }

        auto& root_id() {return root_id_;}
        const auto& root_id() const {return root_id_;}

        void set_root(NodeBaseT* new_root)
        {
            if (root_) {
                root_->unref();
            }

            root_ = new_root;

            if (root_) {
                root_->ref();
            }
        }

        void assign_root_no_ref(NodeBaseT* new_root) {
            root_ = new_root;
        }

        PTreeNodeId new_node_id() {
            return UUID::make_random();
        }

        const auto& parent() const {return parent_;}
        auto& parent() {return parent_;}

        auto& children() {
            return children_;
        }

        const auto& children() const {
            return children_;
        }



        const Status& status() const {
        	return status_;
        }

        void set_status(Status& status) {
        	status_ = status;
        }


        void set_metadata(StringRef metadata) {
        	metadata_ = metadata;
        }

        const auto& metadata() const {
        	HLockGuardT lock(mutex_);
        	return metadata_;
        }

        void commit() {
            status_ = Status::COMMITTED;
        }

        void mark_to_clear() {
            status_ = Status::DROPPED;
        }

        void lock_data() {
        	status_ = Status::DATA_LOCKED;
        }


        auto references()  const
        {
        	return references_;
        }

        auto ref() {
            return ++references_;
        }

        auto unref() {
            return --references_;
        }
    };

    struct HistoryNodeBuffer {

        template <typename, typename>
        friend class PersistentInMemAllocatorT;

    private:
        TxnId parent_;

        String metadata_;

        std::vector<TxnId> children_;

        PTreeNodeId root_;
        typename PageType::ID root_id_;

        typename HistoryNode::Status status_;

        TxnId txn_id_;

    public:

        HistoryNodeBuffer(){}

        const TxnId& txn_id() const {
            return txn_id_;
        }

        TxnId& txn_id() {
            return txn_id_;
        }

        const auto& root() const {return root_;}

        auto& root() {return root_;}

        auto& root_id() {return root_id_;}
        const auto& root_id() const {return root_id_;}

        const auto& parent() const {return parent_;}
        auto& parent() {return parent_;}

        auto& children() {
            return children_;
        }

        const auto& children() const {
            return children_;
        }

        auto& status() {return status_;}
        const auto& status() const {return status_;}

        auto& metadata() {return metadata_;}
        const auto& metadata() const {return metadata_;}
    };


    using PersistentTreeT       = v1::persistent_inmem::PersistentTree<BranchNodeT, LeafNodeT, HistoryNode, PageType>;
    using SnapshotT             = v1::persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersistentTreeT, MyType>;
    using SnapshotPtr           = CtrSharedPtr<SnapshotT>;
    using AllocatorPtr          = std::shared_ptr<MyType>;

    using TxnMap                = std::unordered_map<TxnId, HistoryNode*, UUIDKeyHash, UUIDKeyEq>;

    using HistoryTreeNodeMap    = std::unordered_map<PTreeNodeId, HistoryNodeBuffer*, UUIDKeyHash, UUIDKeyEq>;
    using PersistentTreeNodeMap = std::unordered_map<PTreeNodeId, std::pair<NodeBaseBufferT*, NodeBaseT*>, UUIDKeyHash, UUIDKeyEq>;
    using PageMap               = std::unordered_map<typename PageType::ID, RCPagePtr*, UUIDKeyHash, UUIDKeyEq>;
    using RCPageSet             = std::unordered_set<RCPagePtr*>;
    using BranchMap             = std::unordered_map<String, HistoryNode*>;
    using ReverseBranchMap      = std::unordered_map<const HistoryNode*, String>;

    template <typename, typename, typename, typename, typename>
    friend class v1::persistent_inmem::Snapshot;

private:

    class AllocatorMetadata {
        TxnId master_;
        TxnId root_;

        std::unordered_map<String, TxnId> named_branches_;

    public:
        AllocatorMetadata() {}

        TxnId& master() {return master_;}
        TxnId& root() {return root_;}

        const TxnId& master() const {return master_;}
        const TxnId& root()   const {return root_;}

        auto& named_branches() {return named_branches_;}
        const auto& named_branches() const {return named_branches_;}
    };

    class Checksum {
        BigInt records_;
    public:
        BigInt& records() {return records_;}
        const BigInt& records() const {return records_;}
    };

    enum {TYPE_UNKNOWN, TYPE_METADATA, TYPE_HISTORY_NODE, TYPE_BRANCH_NODE, TYPE_LEAF_NODE, TYPE_DATA_PAGE, TYPE_CHECKSUM};

    Logger logger_;

    HistoryNode* history_tree_ 	= nullptr;
    HistoryNode* master_ 		= nullptr;

    TxnMap snapshot_map_;

    BranchMap named_branches_;

    ContainerMetadataRepository* metadata_;

    BigInt records_ = 0;

    PairPtr pair_;

    mutable MutexT mutex_;
    mutable StoreMutexT store_mutex_;

    CountDownLatch<BigInt> active_snapshots_;

    ReverseBranchMap snapshot_labels_metadata_;

public:
    PersistentInMemAllocatorT():
        logger_("PersistentInMemAllocator"),
        metadata_(MetadataRepository<Profile>::getMetadata())
    {
        SnapshotT::initMetadata();

        master_ = history_tree_ = new HistoryNode(this, HistoryNode::Status::ACTIVE);

        snapshot_map_[history_tree_->txn_id()] = history_tree_;

        auto leaf = new LeafNodeT(history_tree_->txn_id(), UUID::make_random());
        history_tree_->set_root(leaf);

        SnapshotT snapshot(history_tree_, this);
        snapshot.commit();
    }

private:
    PersistentInMemAllocatorT(Int):
        metadata_(MetadataRepository<Profile>::getMetadata())
    {
    	SnapshotT::initMetadata();
    }

    auto& store_mutex() {
    	return store_mutex_;
    }

    const auto& store_mutex() const {
    	return store_mutex_;
    }

public:

    virtual ~PersistentInMemAllocatorT()
    {
        free_memory(history_tree_);
    }

    MutexT& mutex() {
    	return mutex_;
    }

    const MutexT& mutex() const {
    	return mutex_;
    }

    void lock() {
    	mutex_.lock();
    }

    void unlock() {
    	mutex_.unlock();
    }

    bool try_lock() {
    	return mutex_.try_lock();
    }

    PairPtr& pair() {
        return pair_;
    }

    const PairPtr& pair() const {
        return pair_;
    }

    // return true in case of errors
    bool check() {
        return false;
    }

    const Logger& logger() const {
        return logger_;
    }

    Logger& logger() {
        return logger_;
    }

    BigInt active_snapshots() const {
        return active_snapshots_;
    }

    void pack()
    {
        std::lock_guard<MutexT> lock(mutex_);
    	do_pack(history_tree_);
    }


    static auto create()
    {
        return std::make_shared<MyType>();
    }

    auto newId()
    {
        return PageType::ID::make_random();
    }


    persistent_inmem::SnapshotMetadata<TxnId> describe(const TxnId& snapshot_id) const
    {
    	LockGuardT lock_guard2(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            const auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

        	std::vector<TxnId> children;

        	for (const auto& node: history_node->children())
        	{
        		children.emplace_back(node->txn_id());
        	}

        	auto parent_id = history_node->parent() ? history_node->parent()->txn_id() : UUID();

        	return persistent_inmem::SnapshotMetadata<TxnId>(
        		parent_id, history_node->txn_id(), children, history_node->metadata(), history_node->status()
			);
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Snapshot id " << snapshot_id << " is unknown");
        }
    }



    SnapshotPtr find(const TxnId& snapshot_id)
    {
    	LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(snapshot_id);
        if (iter != snapshot_map_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            if (history_node->is_committed())
            {
                return CtrMakeSharedPtr<SnapshotT>::make_shared(history_node, this->shared_from_this());
            }
            if (history_node->is_data_locked())
            {
            	throw Exception(MA_SRC, SBuf() << "Snapshot " << history_node->txn_id() << " data is locked");
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Snapshot " << history_node->txn_id() << " is " << (history_node->is_active() ? "active" : "dropped"));
            }
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Snapshot id " << snapshot_id << " is unknown");
        }
    }




    SnapshotPtr find_branch(StringRef name)
    {
    	LockGuardT lock_guard(mutex_);

    	auto iter = named_branches_.find(name);
        if (iter != named_branches_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            if (history_node->is_committed())
            {
                return CtrMakeSharedPtr<SnapshotT>::make_shared(history_node, this->shared_from_this());
            }
            else if (history_node->is_data_locked())
            {
            	if (history_node->references() == 0)
            	{
            		return CtrMakeSharedPtr<SnapshotT>::make_shared(history_node, this->shared_from_this());
            	}
            	else {
            		throw Exception(MA_SRC, SBuf() << "Snapshot id " << history_node->txn_id() << " is locked and open");
            	}
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Snapshot " << history_node->txn_id() << " is " << (history_node->is_active() ? "active" : "dropped"));
            }
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Named branch \"" << name << "\" is not known");
        }
    }

    SnapshotPtr master()
    {
    	std::lock(mutex_, master_->snapshot_mutex());

    	LockGuardT lock_guard(mutex_, std::adopt_lock);
    	SnapshotLockGuardT snapshot_lock_guard(master_->snapshot_mutex(), std::adopt_lock);

        return CtrMakeSharedPtr<SnapshotT>::make_shared(master_, this->shared_from_this());
    }

    persistent_inmem::SnapshotMetadata<TxnId> describe_master() const
    {
    	std::lock(mutex_, master_->snapshot_mutex());
    	LockGuardT lock_guard2(mutex_, std::adopt_lock);
    	SnapshotLockGuardT snapshot_lock_guard(master_->snapshot_mutex(), std::adopt_lock);

    	std::vector<TxnId> children;

    	for (const auto& node: master_->children())
    	{
    		children.emplace_back(node->txn_id());
    	}

    	auto parent_id = master_->parent() ? master_->parent()->txn_id() : UUID();

    	return persistent_inmem::SnapshotMetadata<TxnId>(
    			parent_id, master_->txn_id(), children, master_->metadata(), master_->status()
    	);
    }

    void set_master(const TxnId& txn_id)
    {
    	LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(txn_id);
        if (iter != snapshot_map_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

            if (history_node->is_committed())
            {
                master_ = iter->second;
            }
            else if (history_node->is_data_locked())
            {
            	master_ = iter->second;
            }
            else if (history_node->is_dropped())
            {
                throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " has been dropped");
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " hasn't been committed yet");
            }
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " is not known in this allocator");
        }
    }

    void set_branch(StringRef name, const TxnId& txn_id)
    {
    	LockGuardT lock_guard(mutex_);

        auto iter = snapshot_map_.find(txn_id);
        if (iter != snapshot_map_.end())
        {
            auto history_node = iter->second;

            SnapshotLockGuardT snapshot_lock_guard(history_node->snapshot_mutex());

        	if (history_node->is_committed())
            {
                named_branches_[name] = history_node;
            }
        	else if (history_node->is_data_locked())
        	{
        		named_branches_[name] = history_node;
        	}
            else {
                throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " hasn't been committed yet");
            }
        }
        else {
            throw Exception(MA_SRC, SBuf()<<"Snapshot " << txn_id << " is not known in this allocator");
        }
    }

    ContainerMetadataRepository* getMetadata() const
    {
        return metadata_;
    }

    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	this->build_snapshot_labels_metadata();

    	LockGuardT lock_guard(mutex_);

        walker->beginAllocator("PersistentInMemAllocator", allocator_descr);

        walk_containers(history_tree_, walker);

        walker->endAllocator();

        snapshot_labels_metadata().clear();
    }

    virtual void store(const char* file)
    {
    	auto fileh = FileOutputStreamHandler::create(file);
        store(fileh.get());
    }

    virtual void store(OutputStreamHandler *output)
    {
    	std::lock(mutex_, store_mutex_);

    	LockGuardT lock_guard2(mutex_, std::adopt_lock);
    	StoreLockGuardT lock_guard1(store_mutex_, std::adopt_lock);

    	active_snapshots_.wait(0);

    	do_pack(history_tree_);

        records_ = 0;

        char signature[12] = "MEMORIA";
        for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

        output->write(&signature, 0, sizeof(signature));

        write_metadata(*output);

        RCPageSet stored_pages;

        walk_version_tree(history_tree_, [&](const HistoryNode* history_tree_node) {
            write_history_node(*output, history_tree_node, stored_pages);
        });

        Checksum checksum;
        checksum.records() = records_;

        write(*output, checksum);

        output->close();
    }

    void dump(const char* path)
    {
        using Walker = FSDumpContainerWalker<Page>;

        Walker walker(this->getMetadata(), path);
        this->walkContainers(&walker);
    }

    static std::shared_ptr<MyType> load(const char* file)
    {
        auto fileh = FileInputStreamHandler::create(file);
        return load(fileh.get());
    }

    static std::shared_ptr<MyType> load(InputStreamHandler *input)
    {
        MyType* allocator = new MyType(0);

        char signature[12];

        input->read(signature, sizeof(signature));

        if (!(
                signature[0] == 'M' &&
                signature[1] == 'E' &&
                signature[2] == 'M' &&
                signature[3] == 'O' &&
                signature[4] == 'R' &&
                signature[5] == 'I' &&
                signature[6] == 'A'))
        {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"The stream does not start from MEMORIA signature: "<<signature);
        }

        if (!(signature[7] == 0 || signature[7] == 1))
        {
            throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Endiannes filed value is out of bounds "<<signature[7]);
        }

        if (signature[8] != 0)
        {
            throw Exception(MEMORIA_SOURCE, "This is not an in-memory container");
        }

        allocator->master_ = allocator->history_tree_ = nullptr;

        allocator->snapshot_map_.clear();

        HistoryTreeNodeMap      history_node_map;
        PersistentTreeNodeMap   ptree_node_map;
        PageMap                 page_map;

        AllocatorMetadata metadata;

        Checksum checksum;

        bool proceed = true;

        while (proceed)
        {
            UByte type;
            *input >> type;

            switch (type)
            {
                case TYPE_METADATA:     allocator->read_metadata(*input, metadata); break;
                case TYPE_DATA_PAGE:    allocator->read_data_page(*input, page_map); break;
                case TYPE_LEAF_NODE:    allocator->read_leaf_node(*input, ptree_node_map); break;
                case TYPE_BRANCH_NODE:  allocator->read_branch_node(*input, ptree_node_map); break;
                case TYPE_HISTORY_NODE: allocator->read_history_node(*input, history_node_map); break;
                case TYPE_CHECKSUM:     allocator->read_checksum(*input, checksum); proceed = false; break;
                default:
                    throw Exception(MA_SRC, SBuf()<<"Unknown record type: "<<(Int)type);
            }

            allocator->records_++;
        }

        if (allocator->records_ != checksum.records())
        {
            throw Exception(MA_SRC, SBuf()<<"Invalid records checksum: actual="<<allocator->records_<<", expected="<<checksum.records());
        }

        for (auto& entry: ptree_node_map)
        {
            auto buffer = entry.second.first;
            auto node   = entry.second.second;

            if (buffer->is_leaf())
            {
                LeafNodeBufferT* leaf_buffer = static_cast<LeafNodeBufferT*>(buffer);
                LeafNodeT* leaf_node         = static_cast<LeafNodeT*>(node);

                for (Int c = 0; c < leaf_node->size(); c++)
                {
                    const auto& descr = leaf_buffer->data(c);

                    auto page_iter = page_map.find(descr.page_ptr());

                    if (page_iter != page_map.end())
                    {
                        leaf_node->data(c) = typename LeafNodeT::Value(page_iter->second, descr.txn_id());
                    }
                    else {
                        throw Exception(MA_SRC, SBuf() << "Specified uuid " << descr.page_ptr() << " is not found in the page map");
                    }
                }
            }
            else {
                BranchNodeBufferT* branch_buffer = static_cast<BranchNodeBufferT*>(buffer);
                BranchNodeT* branch_node         = static_cast<BranchNodeT*>(node);

                for (Int c = 0; c < branch_node->size(); c++)
                {
                    const auto& node_id = branch_buffer->data(c);

                    auto iter = ptree_node_map.find(node_id);

                    if (iter != ptree_node_map.end())
                    {
                        branch_node->data(c) = iter->second.second;
                    }
                    else {
                        throw Exception(MA_SRC, SBuf() << "Specified uuid " << node_id << " is not found in the persistent tree node map");
                    }
                }
            }
        }

        allocator->history_tree_ = allocator->build_history_tree(metadata.root(), nullptr, history_node_map, ptree_node_map);

        if (allocator->snapshot_map_.find(metadata.master()) != allocator->snapshot_map_.end())
        {
            allocator->master_ = allocator->snapshot_map_[metadata.master()];
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Specified master uuid " << metadata.master() << " is not found in the data");
        }

        for (auto& entry: metadata.named_branches())
        {
            auto iter = allocator->snapshot_map_.find(entry.second);

            if (iter != allocator->snapshot_map_.end())
            {
                allocator->named_branches_[entry.first] = iter->second;
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Specified snapshot uuid " << entry.first << " is not found");
            }
        }

        // Delete temporary buffers

        for (auto& entry: ptree_node_map)
        {
            auto buffer = entry.second.first;

            if (buffer->is_leaf())
            {
                static_cast<LeafNodeBufferT*>(buffer)->del();
            }
            else {
                static_cast<BranchNodeBufferT*>(buffer)->del();
            }
        }

        for (auto& entry: history_node_map)
        {
            auto node = entry.second;
            delete node;
        }

        allocator->do_delete_dropped();
        allocator->pack();

        return std::shared_ptr<MyType>(allocator);
    }

private:

    const auto& snapshot_labels_metadata() const {
    	return snapshot_labels_metadata_;
    }

    auto& snapshot_labels_metadata() {
    	return snapshot_labels_metadata_;
    }

    const char* get_labels_for(const HistoryNode* node) const
    {
    	auto labels = snapshot_labels_metadata_.find(node);
    	if (labels != snapshot_labels_metadata_.end())
    	{
    		return labels->second.c_str();
    	}
    	else {
    		return nullptr;
    	}
    }

    void build_snapshot_labels_metadata()
    {
    	snapshot_labels_metadata_.clear();

    	walk_version_tree(history_tree_, [&, this](const HistoryNode* node) {
    		std::vector<String> labels;

    		if (node == history_tree_) {
    			labels.emplace_back("Root");
    		}

    		if (node == master_)
    		{
    			labels.emplace_back("Master Head");
    		}


    		for (const auto& e: named_branches_)
    		{
    			if (e.second == node)
    			{
    				labels.emplace_back(e.first);
    			}
    		}

    		if (labels.size() > 0)
    		{
    			std::stringstream labels_str;

    			bool first = true;
    			for (auto& s: labels)
    			{
    				if (!first)
    				{
    					labels_str << ", ";
    				}
    				else {
    					first = true;
    				}

    				labels_str << s;
    			}

    			snapshot_labels_metadata_[node] = labels_str.str();
    		}
    	});
    }


    HistoryNode* build_history_tree(
        const TxnId& txn_id,
        HistoryNode* parent,
        const HistoryTreeNodeMap& history_map,
        const PersistentTreeNodeMap& ptree_map
    )
    {
        auto iter = history_map.find(txn_id);

        if (iter != history_map.end())
        {
            HistoryNodeBuffer* buffer = iter->second;

            HistoryNode* node = new HistoryNode(this, txn_id, parent, buffer->status());

            node->root_id() = buffer->root_id();
            node->set_metadata(buffer->metadata());

            snapshot_map_[txn_id] = node;

            if (!buffer->root().is_null())
            {
                auto ptree_iter = ptree_map.find(buffer->root());

                if (ptree_iter != ptree_map.end())
                {
                    node->assign_root_no_ref(ptree_iter->second.second);

                    for (const auto& child_txn_id: buffer->children())
                    {
                        build_history_tree(child_txn_id, node, history_map, ptree_map);
                    }

                    return node;
                }
                else {
                    throw Exception(MA_SRC, SBuf() << "Specified node_id " << buffer->root() << " is not found in persistent tree data");
                }
            }
            else {
                for (const auto& child_txn_id: buffer->children())
                {
                    build_history_tree(child_txn_id, node, history_map, ptree_map);
                }

                return node;
            }
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Specified txn_id " << txn_id << " is not found in history data");
        }
    }


    void free_memory(HistoryNode* node)
    {
        for (auto child: node->children())
        {
            free_memory(child);
        }

        if (node->root())
        {
            SnapshotT::delete_snapshot(node);
        }

        delete node;
    }


    /**
     * Deletes recursively all data quickly. Must not be used to delete data of a selected snapshot.
     */

    void free_memory(const NodeBaseT* node)
    {
        const auto& txn_id = node->txn_id();
        if (node->is_leaf())
        {
            auto leaf = PersistentTreeT::to_leaf_node(node);

            for (Int c = 0; c < leaf->size(); c++)
            {
                auto& data = leaf->data(c);
                if (data.txn_id() == txn_id)
                {
                    ::free(data.page());
                }
            }

            leaf->del();
        }
        else {
            auto branch = PersistentTreeT::to_branch_node(node);

            for (Int c = 0; c < branch->size(); c++)
            {
                auto child = branch->data(c);
                if (child->txn_id() == txn_id)
                {
                    free_memory(child);
                }
            }

            branch->del();
        }
    }

    void read_metadata(InputStreamHandler& in, AllocatorMetadata& metadata)
    {
        in >> metadata.master();
        in >> metadata.root();

        BigInt size;

        in >>size;

        for (BigInt c = 0; c < size; c++)
        {
            String name;
            in >> name;

            TxnId value;
            in >> value;

            metadata.named_branches()[name] = value;
        }
    }

    void read_checksum(InputStreamHandler& in, Checksum& checksum)
    {
        in >> checksum.records();
    }

    void read_data_page(InputStreamHandler& in, PageMap& map)
    {
    	typename RCPagePtr::RefCntT references;

    	in >> references;

    	Int page_data_size;
        in >> page_data_size;

        Int page_size;
        in >> page_size;

        Int ctr_hash;
        in >> ctr_hash;

        Int page_hash;
        in >> page_hash;

        unique_ptr<Byte, void (*)(void*)> page_data((Byte*)::malloc(page_data_size), ::free);

        Page* page = T2T<Page*>(::malloc(page_size));

        in.read(page_data.get(), 0, page_data_size);

        auto pageMetadata = metadata_->getPageMetadata(ctr_hash, page_hash);
        pageMetadata->getPageOperations()->deserialize(page_data.get(), page_data_size, T2T<void*>(page));

        if (map.find(page->uuid()) == map.end())
        {
            map[page->uuid()] = new RCPagePtr(page, references);
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Page " << page->uuid() << " was already registered");
        }
    }


    void read_leaf_node(InputStreamHandler& in, PersistentTreeNodeMap& map)
    {
        LeafNodeBufferT* buffer = new LeafNodeBufferT();

        buffer->read(in);

        if (map.find(buffer->node_id()) == map.end())
        {
            NodeBaseT* node = new LeafNodeT();
            node->populate_as_buffer(buffer);

            map[buffer->node_id()] = std::make_pair(buffer, node);
        }
        else {
            throw Exception(MA_SRC, SBuf() << "PersistentTree LeafNode " << buffer->node_id() << " was already registered");
        }
    }


    void read_branch_node(InputStreamHandler& in, PersistentTreeNodeMap& map)
    {
        BranchNodeBufferT* buffer = new BranchNodeBufferT();

        buffer->read(in);

        if (map.find(buffer->node_id()) == map.end())
        {
            NodeBaseT* node = new BranchNodeT();
            node->populate_as_buffer(buffer);

            map[buffer->node_id()] = std::make_pair(buffer, node);
        }
        else {
            throw Exception(MA_SRC, SBuf() << "PersistentTree BranchNode " << buffer->node_id() << " was already registered");
        }
    }




    void read_history_node(InputStreamHandler& in, HistoryTreeNodeMap& map)
    {
        HistoryNodeBuffer* node = new HistoryNodeBuffer();

        Int status;

        in >> status;

        node->status() = static_cast<typename HistoryNode::Status>(status);

        in >> node->txn_id();
        in >> node->root();
        in >> node->root_id();

        in >> node->parent();

        in >> node->metadata();

        BigInt children;
        in >>children;

        for (BigInt c = 0; c < children; c++)
        {
            TxnId child_txn_id;
            in >> child_txn_id;

            node->children().push_back(child_txn_id);
        }

        if (map.find(node->txn_id()) == map.end())
        {
            map[node->txn_id()] = node;
        }
        else {
            throw Exception(MA_SRC, SBuf() << "HistoryTree Node " << node->txn_id() << " was already registered");
        }
    }




//    void update_leaf_references(LeafNodeT* node)
//    {
//        for (Int c = 0; c < node->size(); c++)
//        {
//        	node->data(c)->page_ptr()->raw_data()->references() = node->data(c)->page_ptr()->references();
//        }
//    }



    std::unique_ptr<LeafNodeBufferT> to_leaf_buffer(const LeafNodeT* node)
    {
        std::unique_ptr<LeafNodeBufferT> buf = std::make_unique<LeafNodeBufferT>();

        buf->populate_as_buffer(node);

        for (Int c = 0; c < node->size(); c++)
        {
        	buf->data(c) = typename LeafNodeBufferT::Value(
                node->data(c).page_ptr()->raw_data()->uuid(),
                node->data(c).txn_id()
            );
        }

        return buf;
    }

    std::unique_ptr<BranchNodeBufferT> to_branch_buffer(const BranchNodeT* node)
    {
        std::unique_ptr<BranchNodeBufferT> buf = std::make_unique<BranchNodeBufferT>();

        buf->populate_as_buffer(node);

        for (Int c = 0; c < node->size(); c++)
        {
            buf->data(c) = node->data(c)->node_id();
        }

        return buf;
    }

//  LeafNodeBufferT* to_leaf_buffer(const LeafNodeT* node)
//  {
//      LeafNodeBufferT* buf = new LeafNodeBufferT();
//
//      buf->populate_as_buffer(node);
//
//      for (Int c = 0; c < node->size(); c++)
//      {
//          buf->data(c) = typename LeafNodeBufferT::Value(
//                  node->data(c).page()->uuid(),
//                  node->data(c).txn_id()
//          );
//      }
//
//      return buf;
//  }
//
//  BranchNodeBufferT* to_branch_buffer(const BranchNodeT* node)
//  {
//      BranchNodeBufferT* buf = new BranchNodeBufferT();
//
//      buf->populate_as_buffer(node);
//
//      for (Int c = 0; c < node->size(); c++)
//      {
//          buf->data(c) = node->data(c)->node_id();
//      }
//
//      return buf;
//  }

    void walk_version_tree(HistoryNode* node, std::function<void (HistoryNode*, SnapshotT*)> fn)
    {
        if (node->is_committed())
        {
            SnapshotT txn(node, this);
            fn(node, &txn);
        }

        for (auto child: node->children())
        {
            walk_version_tree(child, fn);
        }
    }

    void walk_version_tree(HistoryNode* node, std::function<void (HistoryNode*)> fn)
    {
        fn(node);

        for (auto child: node->children())
        {
            walk_version_tree(child, fn);
        }
    }

    void walk_containers(HistoryNode* node, ContainerWalker* walker)
    {
    	SnapshotLockGuardT snapshot_lock_guard(node->snapshot_mutex());

        if (node->is_committed())
        {
            SnapshotT txn(node, this);
            txn.walkContainers(walker, get_labels_for(node));
        }

        if (node->children().size())
        {
            walker->beginSnapshotSet("Branches", node->children().size());
            for (auto child: node->children())
            {
                walk_containers(child, walker);
            }
            walker->endSnapshotSet();
        }
    }

    void write_metadata(OutputStreamHandler& out)
    {
        UByte type = TYPE_METADATA;
        out << type;

        out << master_->txn_id();
        out << history_tree_->txn_id();

        out << (BigInt) named_branches_.size();

        for (auto& entry: named_branches_)
        {
            out<<entry.first;
            out<<entry.second->txn_id();
        }

        records_++;
    }

    void write(OutputStreamHandler& out, const Checksum& checksum)
    {
    	UByte type = TYPE_CHECKSUM;
        out << type;
        out << checksum.records() + 1;
    }

    void write_history_node(OutputStreamHandler& out, const HistoryNode* history_node, RCPageSet& stored_pages)
    {
    	UByte type = TYPE_HISTORY_NODE;
        out << type;
        out << (Int)history_node->status();
        out << history_node->txn_id();

        if (history_node->root())
        {
            out << history_node->root()->node_id();
        }
        else {
            out << PTreeNodeId();
        }

        out << history_node->root_id();

        if (history_node->parent())
        {
            out << history_node->parent()->txn_id();
        }
        else {
            out << TxnId();
        }

        out << history_node->metadata();

        out << (BigInt)history_node->children().size();

        for (auto child: history_node->children())
        {
            out << child->txn_id();
        }

        records_++;

        if (history_node->root())
        {
            write_persistent_tree(out, history_node->root(), stored_pages);
        }
    }

    void write_persistent_tree(OutputStreamHandler& out, const NodeBaseT* node, RCPageSet& stored_pages)
    {
        const auto& txn_id = node->txn_id();

        if (node->is_leaf())
        {
            auto leaf = PersistentTreeT::to_leaf_node(node);
            auto buf  = to_leaf_buffer(leaf);

            write(out, buf.get());

            for (Int c = 0; c < leaf->size(); c++)
            {
                const auto& data = leaf->data(c);

                if (stored_pages.count(data.page_ptr()) == 0)
                {
                	write(out, data.page_ptr());
                	stored_pages.insert(data.page_ptr());
                }
            }
        }
        else {
            auto branch = PersistentTreeT::to_branch_node(node);
            auto buf    = to_branch_buffer(branch);

            write(out, buf.get());

            for (Int c = 0; c < branch->size(); c++)
            {
                auto child = branch->data(c);

                if (child->txn_id() == txn_id || child->references() == 1)
                {
                    write_persistent_tree(out, child, stored_pages);
                }
            }
        }
    }


    void write(OutputStreamHandler& out, const BranchNodeBufferT* node)
    {
        UByte type = TYPE_BRANCH_NODE;
        out << type;
        node->write(out);

        records_++;
    }

    void write(OutputStreamHandler& out, const LeafNodeBufferT* node)
    {
        UByte type = TYPE_LEAF_NODE;
        out << type;

        node->write(out);

        records_++;
    }

    void write(OutputStreamHandler& out, const RCPagePtr* page_ptr)
    {
    	auto page = page_ptr->raw_data();

        UByte type = TYPE_DATA_PAGE;
        out << type;

        out << page_ptr->references();

        auto pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        auto page_size = page->page_size();
        unique_ptr<Byte, void (*)(void*)> buffer((Byte*)::malloc(page_size), ::free);

        const auto operations = pageMetadata->getPageOperations();

        Int total_data_size = operations->serialize(page, buffer.get());

        out << total_data_size;
        out << page->page_size();
        out << page->ctr_type_hash();
        out << page->page_type_hash();

        out.write(buffer.get(), 0, total_data_size);

        records_++;
    }


    void dump(const Page* page, std::ostream& out = std::cout)
    {
        TextPageDumper dumper(out);

        auto meta = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        meta->getPageOperations()->generateDataEvents(page, DataEventsParams(), &dumper);
    }

    auto ref_active() {
        return active_snapshots_.inc();
    }

    auto unref_active() {
        return active_snapshots_.dec();
    }

    auto forget_snapshot(HistoryNode* history_node)
    {
    	LockGuardT lock_guard(mutex_);

        snapshot_map_.erase(history_node->txn_id());

        history_node->remove_from_parent();

        delete history_node;
    }

    auto get_named_branch_nodeset() const
    {
    	std::unordered_set<HistoryNode*> set;

    	for (const auto& branch: named_branches_)
    	{
    		set.insert(branch.second);
    	}

    	return set;
    }

    void update_master(HistoryNode* node)
    {
    	if (node->parent())
    	{
    		if (master_ == node)
    		{
    			master_ = node->parent();
    		}
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Null snapshot's parent in do_pack() for snapshot: " << node->txn_id());
    	}
    }

    void do_pack(HistoryNode* node)
    {
        std::unordered_set<HistoryNode*> branches = get_named_branch_nodeset();
    	do_pack(history_tree_, 0, branches);

    	for (const auto& branch: named_branches_)
    	{
    		auto node = branch.second;
    		if (node->root() == nullptr && node->references() == 0)
    		{
    			do_remove_history_node(node);
    		}
    	}
    }

    bool do_remove_history_node(HistoryNode* node)
    {
        if (node->children().size() == 0)
        {
        	update_master(node);

            node->remove_from_parent();
            delete node;

            return true;
        }
        else if (node->children().size() == 1)
        {
        	update_master(node);

        	auto parent = node->parent();
            auto child  = node->children()[0];

            node->remove_from_parent();

            delete node;

            parent->children().push_back(child);
            child->parent() = parent;

            return true;
        }

        return false;
    }

    void do_pack(HistoryNode* node, Int depth, const std::unordered_set<HistoryNode*>& branches)
    {
    	// FIXME: use dedicated stack data structure

        for (auto child: node->children())
        {
            do_pack(child, depth + 1, branches);
        }

        bool remove_node = false;
        {
        	SnapshotLockGuardT lock_guard(node->snapshot_mutex());

        	if (node->root() == nullptr && node->references() == 0 && branches.find(node) == branches.end())
        	{
        		remove_node = true;
        	}
        }

        if (remove_node) {
        	do_remove_history_node(node);
        }
    }

    void do_delete_dropped()
    {
        do_delete_dropped(history_tree_);
    }

    void do_delete_dropped(HistoryNode* node)
    {
        if (node->is_dropped() && node->root() != nullptr)
        {
            SnapshotT::delete_snapshot(node);
        }

        for (auto child: node->children())
        {
            do_delete_dropped(child);
        }
    }

};


template <typename Profile = DefaultProfile<>>
using PersistentInMemAllocator = class PersistentInMemAllocatorT<
        Profile,
        typename ContainerCollectionCfg<Profile>::Types::Page
>;



}}
