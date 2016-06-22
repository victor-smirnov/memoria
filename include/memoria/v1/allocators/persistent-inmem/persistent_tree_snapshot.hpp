
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

#include <memoria/v1/allocators/persistent-inmem/persistent_tree_node.hpp>

#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/exceptions/memoria.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/core/tools/pair.hpp>

#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
namespace v1 {

template <typename Profile, typename PageType>
class PersistentInMemAllocatorT;

namespace persistent_inmem {

namespace {

template <typename CtrT, typename Allocator>
class SharedCtr: public CtrT {

    std::shared_ptr<Allocator> allocator_;

public:
    SharedCtr(const std::shared_ptr<Allocator>& allocator, Int command, const UUID& name):
        CtrT(allocator.get(), command, name),
        allocator_(allocator)
    {}

    auto snapshot() const {
        return allocator_;
    }
};

}

enum class SnapshotStatus {ACTIVE, COMMITTED, DROPPED, DATA_LOCKED};

template <typename TxnId>
class SnapshotMetadata {
	TxnId parent_id_;
	TxnId snapshot_id_;
	std::vector<TxnId> children_;
	String description_;
	SnapshotStatus status_;
public:
	SnapshotMetadata(const TxnId& parent_id, const TxnId& snapshot_id, const std::vector<TxnId>& children, StringRef description, SnapshotStatus status):
		parent_id_(parent_id),
		snapshot_id_(snapshot_id),
		children_(children),
		description_(description),
		status_(status)
	{}

	const TxnId& parent_id() const 			{return parent_id_;}
	const TxnId& snapshot_id() const 		{return snapshot_id_;}
	std::vector<TxnId> children() const 	{return children_;}
	String description() const 				{return description_;}
	SnapshotStatus status() const 			{return status_;}
};



template <typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
class Snapshot:
        public IWalkableAllocator<PageType>,
        public std::enable_shared_from_this<
            Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>
        >
{
    using Base              = IAllocator<PageType>;
    using MyType            = Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>;

    using HistoryTreePtr    = std::shared_ptr<HistoryTree>;
    using SnapshotPtr       = std::shared_ptr<MyType>;

    using NodeBaseT         = typename PersitentTree::NodeBaseT;
    using LeafNodeT         = typename PersitentTree::LeafNodeT;
    using PTreeValue        = typename PersitentTree::LeafNodeT::Value;
    using RCPagePtr			= typename std::remove_pointer<typename PersitentTree::Value::Value>::type;

    using Status            = typename HistoryNode::Status;

    using AllocatorMutexT	= typename std::remove_reference<decltype(std::declval<HistoryNode>().allocator_mutex())>::type;
    using MutexT			= typename std::remove_reference<decltype(std::declval<HistoryNode>().snapshot_mutex())>::type;
    using StoreMutexT		= typename std::remove_reference<decltype(std::declval<HistoryNode>().store_mutex())>::type;

    using LockGuardT			= typename std::lock_guard<MutexT>;
    using StoreLockGuardT		= typename std::lock_guard<StoreMutexT>;
    using AllocatorLockGuardT	= typename std::lock_guard<AllocatorMutexT>;

    class CtrDescr {
    	BigInt references_;
    public:
    	CtrDescr(): references_() {}
    	CtrDescr(BigInt val): references_(val) {}

    	BigInt references() const {return references_;}
    	void ref() 	 {++references_;}
    	BigInt unref() {return --references_;}
    };

    using CtrInstanceMap 	= std::unordered_map<std::type_index, CtrDescr>;

public:

    template <typename CtrName>
    using CtrT = SharedCtr<typename CtrTF<Profile, CtrName>::Type, MyType>;

    template <typename CtrName>
    using CtrPtr = std::shared_ptr<CtrT<CtrName>>;

    using typename Base::Page;
    using typename Base::ID;
    using typename Base::PageG;
    using typename Base::Shared;


private:
    using RootMapType = Ctr<typename CtrTF<Profile, Map<UUID, ID>>::CtrTypes>;


    class Properties: public IAllocatorProperties {
    public:
        virtual Int defaultPageSize() const
        {
            return 8192;
        }

        virtual BigInt lastCommitId() const {
            return 0;
        }

        virtual void setLastCommitId(BigInt txn_id) {}

        virtual BigInt newTxnId() {return 0;}
    };

    HistoryNode*    history_node_;
    HistoryTreePtr  history_tree_;
    HistoryTree*    history_tree_raw_ = nullptr;

    PersitentTree persistent_tree_;

    StaticPool<ID, Shared, 256>  pool_;

    Logger logger_;

    Properties properties_;

    CtrInstanceMap instance_map_;
    std::shared_ptr<RootMapType> root_map_;


    ContainerMetadataRepository*  metadata_;

    template <typename, typename>
    friend class v1::PersistentInMemAllocatorT;

    PairPtr pair_;

public:

    Snapshot(HistoryNode* history_node, const HistoryTreePtr& history_tree):
        history_node_(history_node),
        history_tree_(history_tree),
        history_tree_raw_(history_tree.get()),
        persistent_tree_(history_node_),
        logger_("PersistentInMemAllocatorSnp", Logger::DERIVED, &history_tree->logger_),
        root_map_(nullptr),
        metadata_(MetadataRepository<Profile>::getMetadata())
    {
    	history_node_->ref();

        Int ctr_op;

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
            ctr_op = CTR_CREATE | CTR_FIND;
        }
        else {
            ctr_op = CTR_FIND;
        }

        root_map_ = std::make_shared<RootMapType>(this, ctr_op, UUID());
    }

    Snapshot(HistoryNode* history_node, HistoryTree* history_tree):
        history_node_(history_node),
        history_tree_raw_(history_tree),
        persistent_tree_(history_node_),
        logger_("PersistentInMemAllocatorTxn"),
        root_map_(nullptr),
        metadata_(MetadataRepository<Profile>::getMetadata())
    {
    	history_node_->ref();

        Int ctr_op;

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
            ctr_op = CTR_CREATE | CTR_FIND;
        }
        else {
            ctr_op = CTR_FIND;
        }

        root_map_ = std::make_shared<RootMapType>(this, ctr_op, UUID());
    }

    virtual ~Snapshot()
    {
    	LockGuardT snapshot_lock_guard(history_node_->snapshot_mutex());

        if (history_node_->unref() == 0)
        {
            if (history_node_->is_active())
            {
                do_drop();

                history_tree_raw_->forget_snapshot(history_node_);
            }
            else if(history_node_->is_dropped())
            {
                check_tree_structure(history_node_->root());

                StoreLockGuardT store_lock_guard(history_node_->store_mutex());
                do_drop();
            }
        }
    }

    PairPtr& pair() {
        return pair_;
    }

    const PairPtr& pair() const {
        return pair_;
    }

    static void initMetadata() {
        RootMapType::initMetadata();
    }

    ContainerMetadataRepository* getMetadata() const {
        return metadata_;
    }

    const auto& uuid() const {
        return history_node_->txn_id();
    }

    bool is_active() const {
        return history_node_->is_active();
    }

    bool is_data_locked() const {
    	return history_node_->is_data_locked();
    }

    virtual bool isActive() {
        return is_active();
    }

    bool is_marked_to_clear() const {
        return history_node_->is_dropped();
    }

    bool is_committed() const {
        return history_node_->is_committed();
    }

    SnapshotMetadata<UUID> describe() const
    {
    	std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

    	std::vector<UUID> children;

    	for (const auto& node: history_node_->children())
    	{
    		children.emplace_back(node->txn_id());
    	}

    	auto parent_id = history_node_->parent() ? history_node_->parent()->txn_id() : UUID();

    	return SnapshotMetadata<UUID>(parent_id, history_node_->txn_id(), children, history_node_->metadata(), history_node_->status());
    }

    void commit()
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active())
        {
            history_node_->commit();
            history_tree_raw_->unref_active();
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Invalid Snapshot state: " << (Int)history_node_->status());
        }
    }

    void drop()
    {
    	std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->parent() != nullptr)
        {
            history_node_->mark_to_clear();
        }
        else {
            throw Exception(MA_SRC, "Can't drop root snapshot");
        }
    }

    bool drop_ctr(const UUID& name)
    {
    	checkUpdateAllowed();

        UUID root_id = getRootID(name);

        if (root_id.is_set())
        {
            PageG page = this->getPage(root_id, name);

            auto& ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

            ctr_meta->getCtrInterface()->drop(root_id, name, this);

            return true;
        }
        else {
            return false;
        }
    }

    void set_as_master()
    {
    	history_tree_raw_->set_master(uuid());
    }

    void set_as_branch(StringRef name)
    {
    	history_tree_raw_->set_branch(name, uuid());
    }

    StringRef metadata() const
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());
        return history_node_->metadata();
    }

    void set_metadata(StringRef metadata)
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            throw Exception(MA_SRC, "Snapshot is already committed.");
        }
    }

    bool lock_data_for_export()
    {
    	std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

    	if (history_node_->is_active())
    	{
    		history_node_->commit();
    		history_tree_raw_->unref_active();
    	}

    	if (history_node_->is_committed())
    	{
    		if (history_node_->references() == 0 && history_node_->children().size() == 0)
    		{
    			history_node_->lock_data();
    			return true;
    		}
    		else {
    			return false;
    		}
    	}
    	else if (history_node_->is_data_locked())
    	{
    		return true;
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Invalid Snapshot state: " << (Int)history_node_->status());
    	}
    }

    void unlock_data()
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

    	if (history_node_->is_data_locked())
    	{
    		history_node_->commit();
    	}
    }



    SnapshotPtr branch()
    {
    	std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->is_committed())
        {
            HistoryNode* history_node = new HistoryNode(history_node_);

            LockGuardT lock_guard3(history_node->snapshot_mutex(), std::adopt_lock);

            history_tree_raw_->snapshot_map_[history_node->txn_id()] = history_node;

            return std::make_shared<Snapshot>(history_node, history_tree_->shared_from_this());
        }
        else if (history_node_->is_data_locked())
        {
        	throw Exception(MA_SRC, "Snapshot is locked, branching is not possible.");
        }
        else
        {
            throw Exception(MA_SRC, "Snapshot is still being active. Commit it first.");
        }
    }

    bool has_parent() const
    {
    	AllocatorLockGuardT lock_guard(history_node_->allocator_mutex());

        return history_node_->parent() != nullptr;
    }

    SnapshotPtr parent()
    {
    	std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->parent())
        {
            HistoryNode* history_node = history_node_->parent();
            return std::make_shared<Snapshot>(history_node, history_tree_->shared_from_this());
        }
        else
        {
            throw Exception(MA_SRC, "Snapshot has no parent.");
        }
    }

    void for_each_ctr_node(const UUID& name, typename ContainerInterface::BlockCallbackFn fn)
    {
    	auto root_id = this->getRootID(name);
    	auto page 	 = this->getPage(root_id, name);

    	if (page)
    	{
    		Int master_hash = page->master_ctr_type_hash();
    		Int ctr_hash    = page->ctr_type_hash();

    		auto ctr_meta   = metadata_->getContainerMetadata(master_hash != 0 ? master_hash : ctr_hash);

    		ctr_meta->getCtrInterface()->for_each_ctr_node(name, this, fn);
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Container with name " << name << " does not exist in snapshot " << history_node_->txn_id());
    	}
    }

    void move_new_ctr_from(const SnapshotPtr& txn, const UUID& name)
    {
    	txn->checkIfDataLocked();

    	ID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
    		txn->for_each_ctr_node(name, [&](const UUID& uuid, const UUID& id, const void* page_data){
    			auto rc_handle = txn->export_page(id);
    			using Value = typename PersitentTree::Value;

    			rc_handle->ref();

    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

    			if (old_value.page_ptr())
    			{
    				throw Exception(MA_SRC, SBuf() << "Page with ID " << id << " is not new in snapshot " << txn_id);
    			}
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    			txn->forget_ctr(name);
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Unexpected empty root ID for container " << name << " in snapshot " << txn->currentTxnId());
    		}
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Container with name " << name << " already exists in snapshot " << txn_id);
    	}
    }


    void copy_new_ctr_from(const SnapshotPtr& txn, const UUID& name)
    {
    	ID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
    		txn->for_each_ctr_node(name, [&](const UUID& uuid, const UUID& id, const void* page_data){
    			import_foreign_page(T2T<const Page*>(page_data));
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Unexpected empty root ID for container " << name << " in snapshot " << txn->currentTxnId());
    		}
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Container with name " << name << " already exists in snapshot " << txn_id);
    	}
    }


    void move_ctr_from(const SnapshotPtr& txn, const UUID& name)
    {
    	txn->checkIfDataLocked();

    	ID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (!root_id.is_null())
    	{
    		txn->for_each_ctr_node(name, [&, this](const UUID& uuid, const UUID& id, const void* page_data) {
    			auto page = this->getPage(id, name);
    			if (page && page->uuid() == uuid)
    			{
    				return;
    			}

    			auto rc_handle = txn->export_page(id);
    			using Value = typename PersitentTree::Value;

    			rc_handle->ref();

    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

    			if (old_value.page_ptr())
    			{
    				if (old_value.page_ptr()->unref() == 0)
    				{
    					throw Exception(MA_SRC, SBuf() << "Unexpected refcount == 0 for page " << old_value.page_ptr()->raw_data()->uuid());
    				}
    			}
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Unexpected empty root ID for container " << name << " in snapshot " << txn->currentTxnId());
    		}
    	}
    	else {
    		move_new_ctr_from(txn, name);
    	}
    }


    void copy_ctr_from(const SnapshotPtr& txn, const UUID& name)
    {
    	ID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (!root_id.is_null())
    	{
    		txn->for_each_ctr_node(name, [&, this](const UUID& uuid, const UUID& id, const void* page_data) {
    			auto page = this->getPage(id, name);
    			if (page && page->uuid() == uuid)
    			{
    				return;
    			}

    			import_foreign_page(T2T<const Page*>(page_data));
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    			txn->forget_ctr(name);
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Unexpected empty root ID for container " << name << " in snapshot " << txn_id);
    		}
    	}
    	else {
    		copy_new_ctr_from(txn, name);
    	}
    }


    virtual PageG getPage(const ID& id, const UUID& name)
    {
        if (id.isSet())
        {
            Shared* shared = get_shared(id, Shared::READ);

            if (!shared->get())
            {
                checkReadAllowed();

                auto page_opt = persistent_tree_.find(id);

                if (page_opt)
                {
                    const auto& txn_id = history_node_->txn_id();

                    if (page_opt.value().txn_id() != txn_id)
                    {
                        shared->state() = Shared::READ;
                    }
                    else {
                        shared->state() = Shared::UPDATE;
                    }

                    shared->set_page(page_opt.value().page_ptr()->raw_data());
                }
                else {
                    throw Exception(MA_SRC, SBuf() << "Page is not found for the specified id: " << id);
                }
            }

            return PageG(shared);
        }
        else {
            return PageG();
        }
    }

    void dumpAccess(const char* msg, ID id, const Shared* shared)
    {
        cout<<msg<<": "<<id<<" "<<shared->get()<<" "<<shared->get()->uuid()<<" "<<shared->state()<<endl;
    }


    virtual void registerCtr(const type_info& ti)
    {
    	auto ii = instance_map_.find(ti);
    	if (ii == instance_map_.end())
    	{
    		instance_map_.insert({ti, CtrDescr(1)});
    	}
    	else {
    		ii->second.ref();
    	}
    }

    virtual void unregisterCtr(const type_info& ti)
    {
    	auto ii = instance_map_.find(ti);
    	if (ii == instance_map_.end())
    	{
    		throw Exception(MA_SRC, SBuf() << "Container " << ti.name() << " is not registered in snapshot " << currentTxnId());
    	}
    	else if (ii->second.unref() == 0) {
    		instance_map_.erase(ii);
    	}
    }

    bool has_open_containers() {
    	return instance_map_.size() <= 1;
    }

    void dump_open_containers()
    {
    	for (const auto& pair: instance_map_)
    	{
    		cout << pair.first.name() << " -- " << pair.second.references() << endl;
    	}
    }

    virtual PageG getPageForUpdate(const ID& id, const UUID& name)
    {
        // FIXME: Though this check prohibits new page acquiring for update,
        // already acquired updatable pages can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed();

        if (id.isSet())
        {
            Shared* shared = get_shared(id, Shared::UPDATE);

            if (!shared->get())
            {
                auto page_opt = persistent_tree_.find(id);

                if (page_opt)
                {
                    const auto& txn_id = history_node_->txn_id();

                    if (page_opt.value().txn_id() != txn_id)
                    {
                        Page* new_page = clone_page(page_opt.value().page_ptr()->raw_data());

                        ptree_set_new_page(new_page);

                        shared->set_page(new_page);

                        shared->refresh();
                    }
                    else {
                        MEMORIA_V1_ASSERT(shared->state(), ==, Shared::UPDATE);

                        shared->set_page(page_opt.value().page_ptr()->raw_data());

                        shared->refresh();
                    }
                }
                else {
                    throw Exception(MA_SRC, SBuf() << "Page is not found for the specified id: " << id);
                }
            }
            else if (shared->state() == Shared::READ)
            {
                auto page_opt = persistent_tree_.find(id);

                if (page_opt)
                {
                    Page* new_page = clone_page(page_opt.value().page_ptr()->raw_data());

                    ptree_set_new_page(new_page);
                    shared->set_page(new_page);

                    shared->refresh();
                }
                else {
                    throw Exception(MA_SRC, SBuf() << "Page is not found for the specified id: " << id);
                }
            }
            else if (shared->state() == Shared::UPDATE)
            {
                //MEMORIA_ASEERT();
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Invalid PageShared state: " << shared->state());
            }

            shared->state() = Shared::UPDATE;

            return PageG(shared);
        }
        else {
            return PageG();
        }
    }



    virtual PageG updatePage(Shared* shared, const UUID& name)
    {
        // FIXME: Though this check prohibits new page acquiring for update,
        // already acquired updatable pages can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed();

        if (shared->state() == Shared::READ)
        {
            Page* new_page = clone_page(shared->get());

            ptree_set_new_page(new_page);

            shared->set_page(new_page);

            shared->state() = Shared::UPDATE;

            shared->refresh();
        }

        return PageG(shared);
    }

    virtual void removePage(const ID& id, const UUID& name)
    {
        checkUpdateAllowed();

        auto iter = persistent_tree_.locate(id);

        if (!iter.is_end())
        {
            auto shared = pool_.get(id);

            if (!shared)
            {
                persistent_tree_.remove(iter);
            }
            else {
                shared->state() = Shared::DELETE;
            }
        }
    }






    virtual PageG createPage(Int initial_size, const UUID& name)
    {
        checkUpdateAllowed();

        if (initial_size == -1)
        {
            initial_size = properties_.defaultPageSize();
        }

        void* buf = this->malloc(initial_size);

        memset(buf, 0, initial_size);

        ID id = newId();

        Page* p = new (buf) Page(id);

        p->page_size() = initial_size;

        Shared* shared  = pool_.allocate(id);

        shared->id()    = id;
        shared->state() = Shared::UPDATE;

        shared->set_page(p);
        shared->set_allocator(this);

        ptree_set_new_page(p);

        return PageG(shared);
    }


    virtual void resizePage(Shared* shared, Int new_size)
    {
        checkUpdateAllowed();

        if (shared->state() == Shared::READ)
        {
            Page* page = shared->get();
            auto pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

            Page* new_page = T2T<Page*>(this->malloc(new_size));

            pageMetadata->getPageOperations()->resize(page, new_page, new_size);

            shared->set_page(new_page);

            ptree_set_new_page(new_page);
        }
        else if (shared->state() == Shared::UPDATE)
        {
            Page* page = shared->get();
            auto pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

            Page* new_page  = T2T<Page*>(realloc(page, new_size));

            pageMetadata->getPageOperations()->resize(page, new_page, new_size);

            shared->set_page(new_page);

            ptree_set_new_page(new_page);
        }
    }

    virtual void releasePage(Shared* shared) noexcept
    {
        if (shared->state() == Shared::DELETE)
        {
            persistent_tree_.remove(shared->get()->id());
        }

        pool_.release(shared->id());
    }

    virtual PageG getPageG(Page* page)
    {
        throw Exception(MA_SRC, "Method getPageG is not implemented for this allocator");
    }


    virtual ID newId() {
        return history_tree_raw_->newId();
    }

    virtual UUID currentTxnId() const {
        return history_node_->txn_id();
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return ::malloc(size);
    }
    virtual void  freeMemory(void* ptr) {
        ::free(ptr);
    }

    virtual Logger& logger() {return logger_;}
    virtual IAllocatorProperties& properties() {
        return properties_;
    }

    virtual ID getRootID(const UUID& name)
    {
        if (!name.is_null())
        {
            auto iter = root_map_->find(name);

            if (iter->is_found(name))
            {
                return iter->value();
            }
            else {
                return ID();
            }
        }
        else {
            return history_node_->root_id();
        }
    }

    virtual void setRoot(const UUID& name, const ID& root)
    {
        if (root.is_null())
        {
            if (!name.is_null())
            {
                root_map_->remove(name);
            }
            else {
                throw Exception(MA_SRC, SBuf() << "Allocator directory removal attempted");
            }
        }
        else {
            if (!name.is_null())
            {
                root_map_->assign(name, root);
            }
            else {
                history_node_->root_id() = root;
            }
        }
    }

    virtual void markUpdated(const UUID& name) {}

    virtual bool hasRoot(const UUID& name)
    {
        if (!name.is_null())
        {
            auto iter = root_map_->find(name);
            return iter->is_found(name);
        }
        else {
            return !history_node_->root_id().is_null();
        }
    }

    virtual UUID createCtrName()
    {
        return UUID::make_random();
    }


    virtual bool check()
    {
        bool result = false;

        for (auto iter = root_map_->begin(); !iter->is_end(); )
        {
            auto ctr_name = iter->key();

            PageG page = this->getPage(iter->value(), ctr_name);

            auto ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

            result = ctr_meta->getCtrInterface()->check(page->id(), ctr_name, this) || result;

            iter->next();
        }

        return result;
    }

    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
        walker->beginSnapshot((SBuf() << "Snapshot-" << history_node_->txn_id()).str().c_str());

        auto iter = root_map_->Begin();

        while (!iter->isEnd())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            auto page       = this->getPage(root_id, ctr_name);

            Int master_hash = page->master_ctr_type_hash();
            Int ctr_hash    = page->ctr_type_hash();

            auto ctr_meta   = metadata_->getContainerMetadata(master_hash != 0 ? master_hash : ctr_hash);

            ctr_meta->getCtrInterface()->walk(page->id(), ctr_name, this, walker);

            iter->next();
        }

        walker->endSnapshot();
    }

    void dump_persistent_tree() {
        persistent_tree_.dump();
    }



    template <typename CtrName>
    auto find_or_create(const UUID& name)
    {
        return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_FIND | CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create(const UUID& name)
    {
        return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create()
    {
        return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_CREATE, CTR_DEFAULT_NAME);
    }

    template <typename CtrName>
    auto find(const UUID& name)
    {
        return std::make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_FIND, name);
    }


protected:

    void forget_ctr(const UUID& name)
    {
    	checkIfDataLocked();
    	root_map_->remove(name);
    }

    virtual void forget_page(const ID& id)
    {
    	checkIfDataLocked();

    	auto iter = persistent_tree_.locate(id);

    	if (!iter.is_end())
    	{
    		persistent_tree_.remove(iter);
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Page with id " << id << " does not exist in snapshot " << currentTxnId());
    	}
    }


    auto export_page(const UUID& id)
    {
    	auto opt = persistent_tree_.find(id);

    	if (opt)
    	{
    		persistent_tree_.remove(id, false);
    		return opt.value().page_ptr();
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Page with id " << id << " does not exist in snapshot " << currentTxnId());
    	}
    }


    void import_foreign_page(const Page* foreign_page)
    {
    	Page* new_page = clone_page(foreign_page);
    	ptree_set_new_page(new_page);
    }


    Page* clone_page(const Page* page)
    {
        char* buffer = (char*) this->malloc(page->page_size());

        CopyByteBuffer(page, buffer, page->page_size());
        Page* new_page = T2T<Page*>(buffer);

        new_page->uuid() = newId();

        return new_page;
    }

    Shared* get_shared(Page* page)
    {
        MEMORIA_V1_ASSERT_TRUE(page != nullptr);

        Shared* shared = pool_.get(page->id());

        if (shared == NULL)
        {
            shared = pool_.allocate(page->id());

            shared->id()        = page->id();
            shared->state()     = Shared::UNDEFINED;
            shared->set_page(page);
            shared->set_allocator(this);
        }

        return shared;
    }

    Shared* get_shared(const ID& id, Int state)
    {
        Shared* shared = pool_.get(id);

        if (shared == NULL)
        {
            shared = pool_.allocate(id);

            shared->id()        = id;
            shared->state()     = state;
            shared->set_page((Page*)nullptr);
            shared->set_allocator(this);
        }

        return shared;
    }

    void* malloc(size_t size)
    {
        return ::malloc(size);
    }

    void ptree_set_new_page(Page* page)
    {
        const auto& txn_id = history_node_->txn_id();
        using Value = typename PersitentTree::Value;
        auto ptr = new RCPagePtr(page, 1);
        auto old_value = persistent_tree_.assign(page->id(), Value(ptr, txn_id));

        if (old_value.page_ptr())
        {
            // FIXME: delete page if refs == 0?
        	if (old_value.page_ptr()->unref() == 0) {
        		throw Exception(MA_SRC, SBuf() << "Unexpected refcount == 0 for page " << old_value.page_ptr()->raw_data()->uuid());
        	}
        }
    }


    void checkReadAllowed()
    {
        if (!history_node_->root())
        {
            throw Exception(MA_SRC, "Snapshot has been already cleared");
        }
    }

    void checkUpdateAllowed()
    {
        checkReadAllowed();

        if (!history_node_->is_active())
        {
            throw Exception(MA_SRC, "Snapshot has been already committed");
        }
    }

    void checkIfDataLocked()
    {
    	checkReadAllowed();

    	if (!history_node_->is_data_locked())
    	{
    		throw Exception(MA_SRC, "Snapshot hasn't been locked");
    	}
    }


    void do_drop() throw ()
    {
        persistent_tree_.delete_tree([&](LeafNodeT* leaf){
            for (Int c = 0; c < leaf->size(); c++)
            {
                auto& page_descr = leaf->data(c);
                if (page_descr.page_ptr()->unref() == 0)
                {
                    auto shared = pool_.get(page_descr.page_ptr()->raw_data()->id());

                    if (shared)
                    {
                    	page_descr.page_ptr()->clear();
                        shared->state() = Shared::DELETE;
                    }

                    delete page_descr.page_ptr();
                }
            }
        });
    }

    static void delete_snapshot(HistoryNode* node) throw ()
    {
        PersitentTree persistent_tree(node);

        persistent_tree.delete_tree([&](LeafNodeT* leaf){
            for (Int c = 0; c < leaf->size(); c++)
            {
                auto& page_descr = leaf->data(c);
                if (page_descr.page_ptr()->unref() == 0)
                {
                    delete page_descr.page_ptr();
                }
            }
        });

        node->assign_root_no_ref(nullptr);
        node->root_id() = ID();
    }


    void check_tree_structure(const NodeBaseT* node)
    {
//      if (node->txn_id() == history_node_->txn_id())
//      {
//          if (node->refs() != 1)
//          {
//              cerr << "NodeRefProblem1 for: " << endl;
//              node->dump(cerr);
//          }
//      }
//      else {
//          if (node->refs() < 1)
//          {
//              cerr << "NodeRefProblem2 for: " << endl;
//              node->dump(cerr);
//          }
//      }
//
//      if (node->is_leaf())
//      {
//          //auto leaf_node = PersitentTree::to_leaf_node(node);
//      }
//      else {
//          auto branch_node = PersitentTree::to_branch_node(node);
//
//          for (Int c = 0; c < branch_node->size(); c++)
//          {
//              check_tree_structure(branch_node->data(c));
//          }
//      }
    }
};

}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto create(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc, const UUID& name)
{
    return alloc->template create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto create(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc)
{
    return alloc->template create<CtrName>();
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto find_or_create(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc, const UUID& name)
{
    return alloc->template find_or_create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename HistoryTree>
auto find(const std::shared_ptr<persistent_inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, HistoryTree>>& alloc, const UUID& name)
{
    return alloc->template find<CtrName>(name);
}


template <typename Allocator>
void check_snapshot(const std::shared_ptr<Allocator>& allocator, const char* message,  const char* source)
{
    Int level = allocator->logger().level();

    allocator->logger().level() = Logger::ERROR;

    if (allocator->check())
    {
        allocator->logger().level() = level;

        throw Exception(source, message);
    }

    allocator->logger().level() = level;
}

template <typename Allocator>
void check_snapshot(const std::shared_ptr<Allocator>& allocator)
{
    check_snapshot(allocator, "Snapshot check failed", MA_RAW_SRC);
}



}}
