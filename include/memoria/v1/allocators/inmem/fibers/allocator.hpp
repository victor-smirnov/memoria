
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

#include <memoria/v1/core/types/types.hpp>

#include <dumbo/v1/tools/recursive_mutex.hpp>
#include <dumbo/v1/tools/dumbo_iostreams.hpp>
#include <dumbo/v1/tools/aio_uuid.hpp>
#include <dumbo/v1/tools/aio_string.hpp>
#include <dumbo/v1/tools/sync_file.hpp>
#include <dumbo/v1/tools/dump_walker.hpp>
#include <dumbo/v1/tools/shared_ptr.hpp>
#include <dumbo/v1/tools/memory.hpp>
#include <dumbo/v1/tools/count_down_latch.hpp>
#include <dumbo/v1/tools/misc.hpp>
#include <dumbo/v1/tools/assert.hpp>

#include <dumbo/v1/allocators/inmem/persistent_tree_node.hpp>
#include <dumbo/v1/allocators/inmem/persistent_tree.hpp>
#include <dumbo/v1/allocators/inmem/persistent_tree_snapshot.hpp>

#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/latch.hpp>
#include <memoria/v1/core/tools/strings/string_buffer.hpp>

#include "core/fstream.hh"
#include "core/shared_ptr.hh"
#include "core/sleep.hh"
#include "core/rwlock.hh"

#include <boost/range.hpp>


#include <memory>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include <malloc.h>

namespace dumbo {
namespace v1 {

using namespace memoria::v1;


namespace {

	template <typename PageT>
	struct PagePtr {
		using RefCntT = BigInt;
	private:
		PageT* page_;
		std::atomic<RefCntT> refs_;
		Int cpu_id_;

		// Currently for debug purposes
		static std::atomic<BigInt> page_cnt_;
	public:
		PagePtr(PageT* page, BigInt refs, Int cpu_id): page_(page), refs_(refs), cpu_id_(cpu_id) {}

		~PagePtr() {
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

		Int cpu_id() const {return cpu_id_;}
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
OutputStreamHandler& operator<<(OutputStreamHandler& out, const PersistentTreeValue<V, T>& value)
{
    out << value.page_ptr();
    out << value.txn_id();
    return out;
}

template <typename V, typename T>
InputStreamHandler& operator>>(InputStreamHandler& in, PersistentTreeValue<V, T>& value)
{
    in >> value.page_ptr();
    in >> value.txn_id();
    return in;
}

template <typename V, typename T>
std::ostream& operator<<(std::ostream& out, const PersistentTreeValue<V, T>& value)
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


template <typename V, typename T>
struct AIOWriterAdapter<PersistentTreeValue<V, T>> {
	using TT = PersistentTreeValue<V, T>;

	template <typename OSA>
	static auto process(OSA&& osa, const TT& value)
	{
		return ready(osa << value.page_ptr() << value.txn_id());
	}
};

template <typename V, typename T>
struct AIOReaderAdapter<PersistentTreeValue<V, T>> {
	template <typename ISA>
	static auto process(ISA&& isa)
	{
		return do_with(PersistentTreeValue<V, T>(), [isa](auto& tree_value) {
			return read<V>(isa).then([&, isa](auto vv) {
				tree_value.page_ptr() = vv;
				return read<T>(isa).then([&](auto txn_id) {
					tree_value.txn_id() = txn_id;
					return make_ready_future<PersistentTreeValue<V, T>>(std::move(tree_value));
				});
			});
		});
	}
};



template <typename Profile, typename PageType>
class PersistentInMemAllocatorT: public std::enable_shared_from_this<PersistentInMemAllocatorT<Profile, PageType>> {
public:

    static constexpr Int NodeIndexSize  = 32;
    static constexpr Int NodeSize       = NodeIndexSize * 32;

    using MyType        = PersistentInMemAllocatorT<Profile, PageType>;


    using Page          = PageType;
    using RCPagePtr		= PagePtr<Page>;

    using Key           = typename PageType::ID;
    using Value         = PageType*;

    using TxnId             = UUID;
    using PTreeNodeId       = UUID;

    using LeafNodeT         = v1::inmem::LeafNode<Key, PersistentTreeValue<RCPagePtr*, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
    using LeafNodeBufferT   = v1::inmem::LeafNode<Key, PersistentTreeValue<typename PageType::ID, TxnId>, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;

    using BranchNodeT       = v1::inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId>;
    using BranchNodeBufferT = v1::inmem::BranchNode<Key, NodeSize, NodeIndexSize, PTreeNodeId, TxnId, PTreeNodeId>;
    using NodeBaseT         = typename BranchNodeT::NodeBaseT;
    using NodeBaseBufferT   = typename BranchNodeBufferT::NodeBaseT;

    using TypedAsyncOutputStreamPtr = ::shared_ptr<TypedAsyncOutputStream>;
    using TypedAsyncInputStreamPtr 	= ::shared_ptr<TypedAsyncInputStream>;

    struct HistoryNode {

    	using Status 		= inmem::SnapshotStatus;

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

    template <typename T>
    using SharedForeignPtr 		= dumbo::shared_ptr<T>;


    using PersistentTreeT       = v1::inmem::PersistentTree<BranchNodeT, LeafNodeT, HistoryNode, PageType>;
    using SnapshotT             = v1::inmem::Snapshot<Profile, PageType, HistoryNode, PersistentTreeT, MyType>;
    using SnapshotPtr           = SharedForeignPtr<SnapshotT>;
    using AllocatorPtr          = std::shared_ptr<MyType>;

    using TxnMap                = std::unordered_map<TxnId, HistoryNode*, UUIDKeyHash, UUIDKeyEq>;

    using HistoryTreeNodeMap    = std::unordered_map<PTreeNodeId, HistoryNodeBuffer*, UUIDKeyHash, UUIDKeyEq>;
    using PersistentTreeNodeMap = std::unordered_map<PTreeNodeId, std::pair<NodeBaseBufferT*, NodeBaseT*>, UUIDKeyHash, UUIDKeyEq>;
    using PageMap               = std::unordered_map<typename PageType::ID, RCPagePtr*, UUIDKeyHash, UUIDKeyEq>;
    using RCPageSet             = std::unordered_set<RCPagePtr*>;
    using BranchMap             = std::unordered_map<String, HistoryNode*>;
    using ReverseBranchMap      = std::unordered_map<const HistoryNode*, String>;



    template <typename, typename, typename, typename, typename>
    friend class v1::inmem::Snapshot;

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

    SeastarCountDownLatch<BigInt> active_snapshots_;

//    ReverseBranchMap snapshot_labels_metadata_;

    Int master_cpu_id_;

    rwlock store_mutex_;

public:
    PersistentInMemAllocatorT():
        logger_("PersistentInMemAllocator"),
        metadata_(MetadataRepository<Profile>::getMetadata()),
		master_cpu_id_(engine().cpu_id())
    {
        SnapshotT::initMetadata();

        master_ = history_tree_ = new HistoryNode(this, HistoryNode::Status::ACTIVE);

        snapshot_map_[history_tree_->txn_id()] = history_tree_;

        auto leaf = new LeafNodeT(history_tree_->txn_id(), UUID::make_random(), master_cpu_id_);
        history_tree_->set_root(leaf);

        SnapshotT snapshot(history_tree_, this, engine().cpu_id());
        snapshot.commit();
    }

private:
    PersistentInMemAllocatorT(Int):
    	logger_("PersistentInMemAllocator"),
        metadata_(MetadataRepository<Profile>::getMetadata()),
		master_cpu_id_(engine().cpu_id())
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

    Int cpu_id() const {
    	return master_cpu_id_;
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
        store_mutex_.write_lock().then([this]{
        	do_pack(history_tree_);
        	return ::now();
        }).finally([this] {
        	store_mutex_.write_unlock();
        	return ::now();
        }).get0();
    }


    static auto create()
    {
        return std::make_shared<MyType>();
    }

    auto newId()
    {
    	// FIXME: may block!!!!
        return PageType::ID::make_random();
    }


    SharedForeignPtr<inmem::SnapshotMetadata<TxnId>> describe(const TxnId& snapshot_id) const
    {
    	auto ptr = submit_to_master([=]{
    		auto iter = snapshot_map_.find(snapshot_id);
    		if (iter != snapshot_map_.end())
    		{
    			const auto history_node = iter->second;

    			std::vector<TxnId> children;

    			for (const auto& node: history_node->children())
    			{
    				children.emplace_back(node->txn_id());
    			}

    			auto parent_id = history_node->parent() ? history_node->parent()->txn_id() : UUID();

    			return dumbo::make_shared_holder_now<inmem::SnapshotMetadata<TxnId>>(
    				parent_id, history_node->txn_id(), children, history_node->metadata(), history_node->status()
    			);
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Snapshot id " << snapshot_id << " is unknown");
    		}
    	}).get0();

    	return SharedForeignPtr<inmem::SnapshotMetadata<TxnId>>(std::move(ptr));
    }



    SnapshotPtr find(const TxnId& snapshot_id)
    {
    	Int cpu_id = engine().cpu_id();

    	auto ptr = submit_to_master([=] {
            auto iter = snapshot_map_.find(snapshot_id);
            if (iter != snapshot_map_.end())
            {
                auto history_node = iter->second;

                if (history_node->is_committed())
                {
                    return dumbo::make_shared_holder_now<SnapshotT>(history_node, this->shared_from_this(), cpu_id);
                }
                else if (history_node->is_data_locked())
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
    	}).get0();

    	return SnapshotPtr(std::move(ptr));
    }




    SnapshotPtr find_branch(StringRef name)
    {
    	Int cpu_id = engine().cpu_id();

    	auto ptr = submit_to_master([=] {
    		auto iter = named_branches_.find(name);
    		if (iter != named_branches_.end())
    		{
    			auto history_node = iter->second;

    			if (history_node->is_committed())
    			{
    				return dumbo::make_shared_holder_now<SnapshotT>(history_node, this->shared_from_this(), cpu_id);
    			}
    			else if (history_node->is_data_locked())
    			{
    				if (history_node->references() == 0)
    				{
    					return dumbo::make_shared_holder_now<SnapshotT>(history_node, this->shared_from_this(), cpu_id);
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
    	}).get0();

    	return SnapshotPtr(std::move(ptr));
    }



    SnapshotPtr master()
    {
    	Int owner_cpu_id = engine().cpu_id();
    	auto ptr = submit_to_master([=]{
    		return dumbo::make_shared_holder_now<SnapshotT>(master_, this->shared_from_this(), owner_cpu_id);
    	}).get0();

    	return SnapshotPtr(std::move(ptr));
    }


    SharedForeignPtr<inmem::SnapshotMetadata<TxnId>> describe_master() const
    {
    	auto ptr = submit_to_master([=] {
    		std::vector<TxnId> children;

    		for (const auto& node: master_->children())
    		{
    			children.emplace_back(node->txn_id());
    		}

    		auto parent_id = master_->parent() ? master_->parent()->txn_id() : UUID();

    		return dumbo::make_shared_holder_now<inmem::SnapshotMetadata<TxnId>>(
    			parent_id, master_->txn_id(), children, master_->metadata(), master_->status()
    		);
    	}).get0();

    	return SharedForeignPtr<inmem::SnapshotMetadata<TxnId>>(std::move(ptr));
    }

    void set_master(const TxnId& txn_id)
    {
    	submit_to_master([=]{
    		return store_mutex_.write_lock().then([=]{
    			auto iter = snapshot_map_.find(txn_id);
    			if (iter != snapshot_map_.end())
    			{
    				auto history_node = iter->second;

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

    				return ::now();
    			}
    			else {
    				throw Exception(MA_SRC, SBuf() << "Snapshot " << txn_id << " is not known in this allocator");
    			}
    		}).finally([=]{
    			store_mutex_.write_unlock();
    		});
    	}).get0();
    }

    void set_branch(StringRef name, const TxnId& txn_id)
    {
    	submit_to_master([=]{
    	    return store_mutex_.write_lock().then([=]{
    	        auto iter = snapshot_map_.find(txn_id);
    	        if (iter != snapshot_map_.end())
    	        {
    	            auto history_node = iter->second;

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

    	        	return ::now();
    	        }
    	        else {
    	            throw Exception(MA_SRC, SBuf()<<"Snapshot " << txn_id << " is not known in this allocator");
    	        }
    	    });
    	}).get0();
    }

    ContainerMetadataRepository* getMetadata() const
    {
        return metadata_;
    }

    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	DUMBO_CHECK_THREAD_ACCESS();

    	walkContainers0(walker, allocator_descr);
    }

private:
    void walkContainers0(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	store_mutex_.write_lock().then([=]{
    		auto snapshot_labels_metadata = this->build_snapshot_labels_metadata();

    		walker->beginAllocator("PersistentInMemAllocator", allocator_descr);

    		walk_containers(history_tree_, walker, snapshot_labels_metadata);

    		walker->endAllocator();
    	}).get0();
    }

public:
    virtual void store(const char* dump_file)
    {
    	DUMBO_CHECK_THREAD_ACCESS();

    	auto ff = open_file_dma(dump_file, open_flags::create | open_flags::truncate | open_flags::wo).get0();

    	auto stream = ::dumbo::v1::write(ff);
    	store0(stream);

    	stream->close().get();
    }

    virtual void store(TypedAsyncOutputStreamPtr output) {

    	DUMBO_CHECK_THREAD_ACCESS();

    	store0(output);
    }

private:
    void store0(TypedAsyncOutputStreamPtr output)
    {
    	store_mutex_.write_lock().then([=]{
    		return active_snapshots_.wait().then([=]{

    			do_pack(history_tree_);

    			records_ = 0;

    			char signature[12] = "MEMORIA";
    			for (size_t c = 7; c < sizeof(signature); c++) signature[c] = 0;

    			output->write(signature, 0, sizeof(signature)).get();

    			std::cout << "Header\n";

    			write_metadata(output);

    			RCPageSet stored_pages;

    			walk_version_tree(history_tree_, [&](const HistoryNode* history_tree_node) {
    				write_history_node(output, history_tree_node, stored_pages);
    			});

    			Checksum checksum;
    			checksum.records() = records_;

    			write(output, checksum);

    			output->flush().get0();
    		});
    	}).finally([=]{
    		store_mutex_.write_unlock();
    		return ::now();
    	})
		.get0();
    }
public:


    void dump(const char* path)
    {
    	submit_async_to_master([=]{
            using Walker = DumboFSDumpContainerWalker<Page>;

            Walker walker(this->getMetadata(), path);
            this->walkContainers0(&walker);
    	}).get0();
    }

    static std::shared_ptr<MyType> load(const char* file)
    {
    	auto ff = open_file_dma(file, open_flags::ro).get0();

    	auto stream = ::dumbo::v1::read(ff);
    	auto ptr = load(stream);

    	stream->close().get();

    	return ptr;
    }

    static std::shared_ptr<MyType> load(TypedAsyncInputStreamPtr input)
    {
    	MyType* allocator = new MyType(0);

        char signature[12];

        input->read(signature, sizeof(signature)).get();

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
            (input >> type).get();

            switch (type)
            {
                case TYPE_METADATA:     allocator->read_metadata(input, metadata); break;
                case TYPE_DATA_PAGE:    allocator->read_data_page(input, page_map); break;
                case TYPE_LEAF_NODE:    allocator->read_leaf_node(input, ptree_node_map); break;
                case TYPE_BRANCH_NODE:  allocator->read_branch_node(input, ptree_node_map); break;
                case TYPE_HISTORY_NODE: allocator->read_history_node(input, history_node_map); break;
                case TYPE_CHECKSUM:     allocator->read_checksum(input, checksum); proceed = false; break;
                default:
                    throw Exception(MA_SRC, SBuf() << "Unknown record type: " << (Int)type);
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

        Int cpu_id = allocator->cpu_id();

        for (auto& entry: ptree_node_map)
        {
            auto buffer = entry.second.first;

            if (buffer->is_leaf())
            {
                static_cast<LeafNodeBufferT*>(buffer)->del(cpu_id);
            }
            else {
                static_cast<BranchNodeBufferT*>(buffer)->del(cpu_id);
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

    template <typename Fn>
    auto submit_to_master(Fn&& fn) const
	{
    	return smp::submit_to(master_cpu_id_, std::forward<Fn>(fn));
    }

    template <typename Fn>
    future<> submit_async_to_master(Fn&& fn) const
	{
    	return smp::submit_to(master_cpu_id_, [&, fn = std::forward<Fn>(fn)]{
    		if (seastar::thread::running_in_thread()) {
    			fn();
    			return ::now();
    		}
    		else {
    			return seastar::async(fn);
    		}
    	});
	}


//    const auto& snapshot_labels_metadata() const {
//    	return snapshot_labels_metadata_;
//    }
//
//    auto& snapshot_labels_metadata() {
//    	return snapshot_labels_metadata_;
//    }

    const char* get_labels_for(const HistoryNode* node, const ReverseBranchMap& snapshot_labels_metadata) const
    {
    	auto labels = snapshot_labels_metadata.find(node);
    	if (labels != snapshot_labels_metadata.end())
    	{
    		return labels->second.c_str();
    	}
    	else {
    		return nullptr;
    	}
    }

    ReverseBranchMap build_snapshot_labels_metadata()
    {
    	ReverseBranchMap snapshot_labels_metadata;

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

    			snapshot_labels_metadata[node] = labels_str.str();
    		}
    	});

    	return snapshot_labels_metadata;
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

    void read_metadata(TypedAsyncInputStreamPtr in, AllocatorMetadata& metadata)
    {
    	ready(in >> metadata.master()
           >> metadata.root()).then([=, &metadata] {
        	return read<BigInt>(in).then([=, &metadata](auto size) {
        		return do_with(boost::irange((BigInt)0, (BigInt)size, (BigInt)1), [&](auto& range) {
        			return do_for_each(range, [=, &metadata](auto idx) {
        				return read<String>(in).then([=, &metadata](auto name) {
        					return read<TxnId>(in).then([=, &metadata, name = std::move(name)](auto value) {
        						metadata.named_branches()[name] = value;
        						return ::now();
        					});
        				});
        			});
        		});
        	});
        }).get();
    }

    void read_checksum(TypedAsyncInputStreamPtr in, Checksum& checksum)
    {
        (in >> checksum.records()).get();
    }

    void read_data_page(TypedAsyncInputStreamPtr in, PageMap& map)
    {
        read<typename RCPagePtr::RefCntT>(in).then([=, &map](auto references){
        	return read<Int>(in).then([=, &map](auto page_data_size) {
        		return read<Int>(in).then([=, &map](auto page_size) {
        			return read<Int>(in).then([=, &map](auto ctr_hash) {
        				return read<Int>(in).then([=, &map](auto page_hash) {

        					Byte* page_data_ptr = (Byte*)::malloc(page_data_size);
        					unique_ptr<Byte, void (*)(void*)> page_data(page_data_ptr, ::free);

        					return in->read(page_data_ptr, page_data_size).then([=, &map, page_data = std::move(page_data)] {

        						Page* page = T2T<Page*>(::malloc(page_size));

        						auto pageMetadata = metadata_->getPageMetadata(ctr_hash, page_hash);
        						pageMetadata->getPageOperations()->deserialize(page_data.get(), page_data_size, T2T<void*>(page));

        						if (map.find(page->uuid()) == map.end())
        						{
        							map[page->uuid()] = new RCPagePtr(page, references, master_cpu_id_);
        						}
        						else {
        							throw Exception(MA_SRC, SBuf() << "Page " << page->uuid() << " was already registered");
        						}

        						return ::now();
        					});
        				});
        			});
        		});
        	});
        }).get();
    }


    void read_leaf_node(TypedAsyncInputStreamPtr in, PersistentTreeNodeMap& map)
    {
        LeafNodeBufferT* buffer = new LeafNodeBufferT();

        buffer->read(in).then([=, &map]{
        	if (map.find(buffer->node_id()) == map.end())
        	{
        		NodeBaseT* node = new LeafNodeT();
        		node->populate_as_buffer(buffer);

        		map[buffer->node_id()] = std::make_pair(buffer, node);
        	}
        	else {
        		throw Exception(MA_SRC, SBuf() << "PersistentTree LeafNode " << buffer->node_id() << " was already registered");
        	}
        }).get();
    }


    void read_branch_node(TypedAsyncInputStreamPtr in, PersistentTreeNodeMap& map)
    {
        BranchNodeBufferT* buffer = new BranchNodeBufferT();

        buffer->read(in).then([=, &map]{
        	if (map.find(buffer->node_id()) == map.end())
        	{
        		NodeBaseT* node = new BranchNodeT();
        		node->populate_as_buffer(buffer);

        		map[buffer->node_id()] = std::make_pair(buffer, node);
        	}
        	else {
        		throw Exception(MA_SRC, SBuf() << "PersistentTree BranchNode " << buffer->node_id() << " was already registered");
        	}
        }).get();
    }




    void read_history_node(TypedAsyncInputStreamPtr in, HistoryTreeNodeMap& map)
    {
        HistoryNodeBuffer* node = new HistoryNodeBuffer();

        Int status;
        BigInt children;

        ready(in >> status
        	>> node->txn_id()
			>> node->root()
        	>> node->root_id()
			>> node->parent()
			>> node->metadata()
			>> children
		).then([=, &children, &status]{

        	node->status() = static_cast<typename HistoryNode::Status>(status);

			BigInt lchildren = children;

			return do_with(boost::irange((BigInt)0, (BigInt)lchildren, (BigInt)1), [=](auto& range) {
				return do_for_each(range, [=](auto idx) {
					return read<TxnId>(in).then([=](const auto& child_txn_id) {
						node->children().push_back(child_txn_id);
						return ::now();
					});
				});
			});
		}).get();

        if (map.find(node->txn_id()) == map.end())
        {
        	map[node->txn_id()] = node;
        }
        else {
        	throw Exception(MA_SRC, SBuf() << "HistoryTree Node " << node->txn_id() << " was already registered");
        }
    }


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

    void walk_containers(HistoryNode* node, ContainerWalker* walker, const ReverseBranchMap& snapshot_labels_metadata)
    {
        if (node->is_committed())
        {
            SnapshotT txn(node, this, engine().cpu_id());
            txn.walkContainers(walker, get_labels_for(node, snapshot_labels_metadata));
        }

        if (node->children().size())
        {
            walker->beginSnapshotSet("Branches", node->children().size());
            for (auto child: node->children())
            {
                walk_containers(child, walker, snapshot_labels_metadata);
            }
            walker->endSnapshotSet();
        }
    }

    void write_metadata(const TypedAsyncOutputStreamPtr& out)
    {
    	UByte type = TYPE_METADATA;

        ready(
        	out << type
        		<< master_->txn_id()
				<< history_tree_->txn_id()
				<< (BigInt) named_branches_.size()
		).then([=]{
        	return do_for_each(named_branches_, [=](const auto& entry){
        		return ready(
        			out << entry.first
        				<< entry.second->txn_id());
        	});
        }).get();

        records_++;
    }

    void write(TypedAsyncOutputStreamPtr out, const Checksum& checksum)
    {
    	UByte type = TYPE_CHECKSUM;

        ready(
        	out << type
        		<< checksum.records() + 1
		).get();
    }

    void write_history_node(const TypedAsyncOutputStreamPtr& out, const HistoryNode* history_node, RCPageSet& stored_pages)
    {
    	UByte type = TYPE_HISTORY_NODE;

    	((ready(
    		out << type
				<< (Int)history_node->status()
				<< history_node->txn_id()
        ).then([=]{
    		if (history_node->root())
    		{
    			return out << history_node->root()->node_id();
    		}
    		else {
    			return out << PTreeNodeId();
    		}
        })
			<< history_node->root_id()
		).then([=](auto out) {
    		if (history_node->parent())
    		{
    			return out << history_node->parent()->txn_id();
    		}
    		else {
    			return out << TxnId();
    		}
    	})
			<< history_node->metadata()
			<< (BigInt)history_node->children().size()
    	).then([=](auto out) {
    		return do_for_each(history_node->children(), [=](const auto& child) {
    			return ready(out << child->txn_id());
    		});
    	}).get();

        records_++;

        if (history_node->root())
        {
            write_persistent_tree(out, history_node->root(), stored_pages);
        }
    }

    void write_persistent_tree(const TypedAsyncOutputStreamPtr& out, const NodeBaseT* node, RCPageSet& stored_pages)
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


    void write(TypedAsyncOutputStreamPtr out, const BranchNodeBufferT* node)
    {
        UByte type = TYPE_BRANCH_NODE;

        (out << type).then([=](auto out){
        	return node->write(out);
        }).get();

        records_++;
    }

    void write(TypedAsyncOutputStreamPtr out, const LeafNodeBufferT* node)
    {
    	UByte type = TYPE_LEAF_NODE;

        (out << type).then([=](auto out){
        	return node->write(out);
        }).get();

        records_++;
    }

    void write(TypedAsyncOutputStreamPtr out, const RCPagePtr* page_ptr)
    {
    	UByte type = TYPE_DATA_PAGE;

    	auto page = page_ptr->raw_data();

        auto pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

        auto page_size = page->page_size();
        unique_ptr<Byte, void (*)(void*)> buffer((Byte*)::malloc(page_size), ::free);

        const auto operations = pageMetadata->getPageOperations();

        Int total_data_size = operations->serialize(page, buffer.get());

        (out << type
        	<< page_ptr->references()
			<< total_data_size
			<< page->page_size()
			<< page->ctr_type_hash()
			<< page->page_type_hash()
		).then([=, &buffer](auto out) {
        	return out->write(buffer.get(), 0, total_data_size);
        }).get();

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

    void forget_snapshot(HistoryNode* history_node)
    {
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

    void checkThreadAccess(const char* source, unsigned int line, const char* function) const
    {
    	Int cpu_id = engine().cpu_id();

    	if (master_cpu_id_ != cpu_id)
    	{
    		__assert_fail(
    				(SBuf()
    						<< "SeastarInMemAllocator thread access assertion failed. Master thread: " << master_cpu_id_
							<< "Caller thread: " << cpu_id).str().c_str(),
					source,
					line,
					function
			);
    	}
    }
};


template <typename Profile = DumboProfile<>>
using SeastarInMemAllocator = class PersistentInMemAllocatorT<
        Profile,
        typename ContainerCollectionCfg<Profile>::Types::Page
>;



}}
