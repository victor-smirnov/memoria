
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

#include <dumbo/v1/allocators/inmem/persistent_tree_node.hpp>
#include <dumbo/v1/tools/memory.hpp>

#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/exceptions/memoria.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#include <vector>
#include <memory>
#include <mutex>



namespace dumbo {
namespace v1 {

using namespace memoria::v1;


template <typename Profile, typename PageType>
class PersistentInMemAllocatorT;

namespace inmem {

namespace {

	template <typename CtrT, typename Allocator>
	class SharedCtr: public CtrT {
		dumbo::shared_ptr<Allocator> allocator_;

	public:
		SharedCtr(const dumbo::shared_ptr<Allocator>& allocator, Int command, const UUID& name):
			CtrT(allocator.get(), command, name),
			allocator_(allocator)
		{}

		virtual ~SharedCtr() {}

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



template <typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename Allocator>
class Snapshot:
        public IWalkableAllocator<PageType>,
        public dumbo::enable_shared_from_this<
            Snapshot<Profile, PageType, HistoryNode, PersitentTree, Allocator>
        >
{
    using Base              = IAllocator<PageType>;
    using MyType            = Snapshot<Profile, PageType, HistoryNode, PersitentTree, Allocator>;

    template <typename T>
    using SharedForeignPtr 	= typename Allocator::template SharedForeignPtr<T>;

    using AllocatorPtr    	= std::shared_ptr<Allocator>;
    using SnapshotPtr       = SharedForeignPtr<MyType>;

    using NodeBaseT         = typename PersitentTree::NodeBaseT;
    using LeafNodeT         = typename PersitentTree::LeafNodeT;
    using PTreeValue        = typename PersitentTree::LeafNodeT::Value;
    using RCPagePtr			= typename std::remove_pointer<typename PersitentTree::Value::Value>::type;

    using Status            = typename HistoryNode::Status;

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

    template <typename T>
    using CtrMakeSharedPtr 	= memoria::v1::CtrMakeSharedPtr<Profile, T>;

    template <typename T>
    using CtrSharedPtr 		= memoria::v1::CtrSharedPtr<Profile, T>;

    template <typename CtrName>
    using CtrT = SharedCtr<typename CtrTF<Profile, CtrName>::Type, MyType>;


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

    HistoryNode*  history_node_;
    AllocatorPtr  allocator_;
    Allocator*    allocator_raw_ = nullptr;

    PersitentTree persistent_tree_;

    StaticPool<ID, Shared, 256>  pool_;

    Logger logger_;

    Properties properties_;

    CtrInstanceMap instance_map_;
    CtrSharedPtr<RootMapType> root_map_;


    ContainerMetadataRepository*  metadata_;

    template <typename, typename>
    friend class v1::PersistentInMemAllocatorT;

    PairPtr pair_;

    Int cpu_id_;

public:

    Snapshot(HistoryNode* history_node, const AllocatorPtr& history_tree, Int cpu_id):
        history_node_(history_node),
        allocator_(history_tree),
        allocator_raw_(history_tree.get()),
        persistent_tree_(history_node_, cpu_id),
        logger_("PersistentInMemAllocatorSnp", Logger::DERIVED, &history_tree->logger_),
        root_map_(nullptr),
        metadata_(MetadataRepository<Profile>::getMetadata()),
		cpu_id_(cpu_id)
    {
    	history_node_->ref();

        Int ctr_op;

        if (history_node->is_active())
        {
            allocator_raw_->ref_active();
            ctr_op = CTR_CREATE | CTR_FIND;
        }
        else {
            ctr_op = CTR_FIND;
        }

        root_map_ = CtrMakeSharedPtr<RootMapType>::make_shared(this, ctr_op, UUID());
    }

    Snapshot(HistoryNode* history_node, Allocator* history_tree, Int cpu_id):
        history_node_(history_node),
        allocator_raw_(history_tree),
        persistent_tree_(history_node_, cpu_id),
        logger_("PersistentInMemAllocatorTxn"),
        root_map_(nullptr),
        metadata_(MetadataRepository<Profile>::getMetadata()),
		cpu_id_(cpu_id)
    {
    	history_node_->ref();

        Int ctr_op;

        if (history_node->is_active())
        {
            allocator_raw_->ref_active();
            ctr_op = CTR_CREATE | CTR_FIND;
        }
        else {
            ctr_op = CTR_FIND;
        }

        root_map_ = CtrMakeSharedPtr<RootMapType>::make_shared(this, ctr_op, UUID());
    }



    virtual ~Snapshot()
    {
    	if (history_node_->unref() == 0)
    	{
    		if (history_node_->is_active())
    		{
    			do_drop_active();
    		}
    		else if(history_node_->is_dropped())
    		{
    			do_drop_dropped();
    		}
    		else if (history_node_->is_committed())
    		{
    			allocator_raw_->unref_active();
    		}
    		else {
    			assert(true && "Unexpected snapshot state in Snapshot's destructor");
    		}
    	}
    }




    Int cpu_id() const {
    	return cpu_id_;
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

    SharedForeignPtr<SnapshotMetadata<UUID>> describe() const
    {
    	using T = SnapshotMetadata<UUID>;

    	auto ptr = allocator_raw_->submit_to_master([=]{
        	std::vector<UUID> children;

        	for (const auto& node: history_node_->children())
        	{
        		children.emplace_back(node->txn_id());
        	}

        	auto parent_id = history_node_->parent() ? history_node_->parent()->txn_id() : UUID();

        	return dumbo::make_shared_holder_now<T>(
        		parent_id, history_node_->txn_id(), children, history_node_->metadata(), history_node_->status()
			);
    	}).get0();

    	return SharedForeignPtr<T>(std::move(ptr));
    }

    void commit()
    {
    	allocator_raw_->submit_to_master([=]{
    		if (history_node_->is_active() || history_node_->is_data_locked())
    		{
    			history_node_->commit();
    			return ::now();
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Invalid state: " << (Int)history_node_->status() << " for snapshot " << uuid());
    		}
    	}).get0();
    }

    void drop()
    {
    	allocator_raw_->submit_to_master([=]{
    		if (history_node_->parent() != nullptr)
    		{
    			history_node_->mark_to_clear();
    			return ::now();
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Can't drop root snapshot " << uuid());
    		}
    	}).get0();
    }

    bool drop_ctr(const UUID& name)
    {
    	DUMBO_CHECK_THREAD_ACCESS();

    	checkUpdateAllowed(name);

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
    	allocator_raw_->set_master(uuid());
    }

    void set_as_branch(StringRef name)
    {
    	allocator_raw_->set_branch(name, uuid());
    }

    String metadata() const
    {
    	auto ptr = allocator_raw_->submit_to_master([=] {
    		return dumbo::make_shared_holder_now<String>(history_node_->metadata());
    	}).get0();

    	return String(*SharedForeignPtr<String>(std::move(ptr)).get());
    }

    void set_metadata(StringRef metadata)
    {
    	allocator_raw_->submit_to_master([=]{
            if (history_node_->is_active())
            {
                history_node_->set_metadata(metadata);
                return ::now();
            }
            else
            {
                throw Exception(MA_SRC, "Snapshot is already committed.");
            }
    	}).get0();
    }

    void lock_data_for_import()
    {
    	allocator_raw_->submit_to_master([=]{
    		if (history_node_->is_active())
    		{
    			if (!has_open_containers())
    			{
    				history_node_->lock_data();
    			}
    			else {
    				throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " has open containers");
    			}
    		}
    		else if (history_node_->is_data_locked()) {
    		}
    		else {
    			throw Exception(MA_SRC, SBuf() << "Invalid state: " << (Int)history_node_->status() << " for snapshot " << uuid());
    		}

    		return ::now();
    	}).get0();
    }


    SnapshotPtr branch()
    {
    	Int owner_cpu_id = engine().cpu_id();

    	auto holder = allocator_raw_->submit_to_master([=]{
    		return allocator_raw_->store_mutex_.write_lock().then([=]{

    			allocator_raw_->ref_active();

    			if (history_node_->is_committed())
    			{
    				HistoryNode* history_node = new HistoryNode(history_node_);

    				allocator_raw_->snapshot_map_[history_node->txn_id()] = history_node;

    				return dumbo::make_shared_holder_now<Snapshot>(history_node, allocator_->shared_from_this(), owner_cpu_id);
    			}
    			else if (history_node_->is_data_locked())
    			{
    				throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " is locked, branching is not possible.");
    			}
    			else
    			{
    				throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " is still being active. Commit it first.");
    			}
    		}).finally([=]{
    			allocator_raw_->store_mutex_.write_unlock();
    		});
    	}).get0();

        return SnapshotPtr(std::move(holder));
    }

    bool has_parent() const
    {
    	return allocator_raw_->submit_to_master([=]{
    		return make_ready_future<bool>(history_node_->parent() != nullptr);
    	}).get0();
    }

    SnapshotPtr parent()
    {
    	Int owner_cpu_id = engine().cpu_id();

    	auto holder = allocator_raw_->submit_to_master([=]{
    		if (history_node_->parent())
    		{
    			HistoryNode* history_node = history_node_->parent();

    			return dumbo::make_shared_holder_now<Snapshot>(
    				history_node, allocator_->shared_from_this(), owner_cpu_id
    			);
    		}
    		else
    		{
    			throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " has no parent.");
    		}
    	});

    	return SnapshotPtr(std::move(holder.get0()));
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



    void import_new_ctr_from(const SnapshotPtr& txn, const UUID& name)
    {
    	DUMBO_CHECK_THREAD_ACCESS();

    	checkIfDataLocked();
    	txn->checkIfExportAllowed();

    	ID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
    		txn->for_each_ctr_node(name, [&](const UUID& uuid, const UUID& id, const void* page_data){
    			auto rc_handle = txn->export_page_rchandle(id);
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
    	DUMBO_CHECK_THREAD_ACCESS();

    	txn->checkReadAllowed();
    	checkUpdateAllowed();

    	ID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
    		txn->for_each_ctr_node(name, [&](const UUID& uuid, const UUID& id, const void* page_data){
    			clone_foreign_page(T2T<const Page*>(page_data));
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







    void import_ctr_from(const SnapshotPtr& txn, const UUID& name)
    {
    	DUMBO_CHECK_THREAD_ACCESS();

    	checkIfDataLocked();
    	txn->checkIfExportAllowed();

    	ID root_id = this->getRootID(name);

    	auto txn_id = uuid();

    	if (!root_id.is_null())
    	{
    		txn->for_each_ctr_node(name, [&, this](const UUID& uuid, const UUID& id, const void* page_data) {
    			auto page = this->getPage(id, name);
    			if (page && page->uuid() == uuid)
    			{
    				return;
    			}

    			auto rc_handle = txn->export_page_rchandle(id);
    			using Value = typename PersitentTree::Value;

    			rc_handle->ref();

    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

    			if (old_value.page_ptr())
    			{
    				if (old_value.page_ptr()->unref() == 0)
    				{
    					// FIXME: just delete the page?
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
    		import_new_ctr_from(txn, name);
    	}
    }


    void copy_ctr_from(const SnapshotPtr& txn, const UUID& name)
    {
    	DUMBO_CHECK_THREAD_ACCESS();

    	txn->checkReadAllowed();
    	checkUpdateAllowed();

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

    			clone_foreign_page(T2T<const Page*>(page_data));
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
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
    	DUMBO_CHECK_THREAD_ACCESS();

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
    		throw Exception(MA_SRC, SBuf() << "Container " << ti.name() << " is not registered in snapshot " << uuid());
    	}
    	else if (ii->second.unref() == 0) {
    		instance_map_.erase(ii);
    	}
    }

    bool has_open_containers() {
    	return instance_map_.size() > 1;
    }

    void dump_open_containers()
    {
    	for (const auto& pair: instance_map_)
    	{
    		cout << demangle(pair.first.name()) << " -- " << pair.second.references() << endl;
    	}
    }

    virtual PageG getPageForUpdate(const ID& id, const UUID& name)
    {
    	DUMBO_CHECK_THREAD_ACCESS();

        // FIXME: Though this check prohibits new page acquiring for update,
        // already acquired updatable pages can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(name);

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
    	DUMBO_CHECK_THREAD_ACCESS();

        // FIXME: Though this check prohibits new page acquiring for update,
        // already acquired updatable pages can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(name);

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
    	DUMBO_CHECK_THREAD_ACCESS();

        checkUpdateAllowed(name);

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
    	DUMBO_CHECK_THREAD_ACCESS();

        checkUpdateAllowed(name);

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
    	DUMBO_CHECK_THREAD_ACCESS();

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
    	DUMBO_CHECK_THREAD_ACCESS();

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
        return allocator_raw_->newId();
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
    	DUMBO_CHECK_THREAD_ACCESS();

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
    	DUMBO_CHECK_THREAD_ACCESS();

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
    	DUMBO_CHECK_THREAD_ACCESS();

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
    	DUMBO_CHECK_THREAD_ACCESS();

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

    String get_branch_suffix() const
    {
    	return std::string("");
    }

    virtual void walkContainers(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	DUMBO_CHECK_THREAD_ACCESS();
    	walkContainers0(walker, allocator_descr);
    }

private:
    void walkContainers0(ContainerWalker* walker, const char* allocator_descr = nullptr)
    {
    	if (allocator_descr != nullptr)
		{
			walker->beginSnapshot((SBuf() << "Snapshot-" << history_node_->txn_id() << " -- " << allocator_descr).str().c_str());
		}
		else {
			walker->beginSnapshot((SBuf() << "Snapshot-" << history_node_->txn_id()).str().c_str());
		}

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
public:

    void dump(const char* destination)
    {
    	return submit_async_to(cpu_id_, [=]{
    		using Walker = FSDumpContainerWalker<Page>;

    		Walker walker(this->getMetadata(), destination);

    		auto lebels_metadata = history_node_->allocator()->build_snapshot_labels_metadata();

    		walkContainers0(
    			&walker,
				history_node_->allocator()->get_labels_for(history_node_, lebels_metadata)
			);
    	}).get0();
    }

    void dump_persistent_tree()
    {
    	DUMBO_CHECK_THREAD_ACCESS();
        persistent_tree_.dump_tree();
    }



    template <typename CtrName>
    auto find_or_create(const UUID& name)
    {
    	checkIfConainersCreationAllowed();
        return CtrMakeSharedPtr<CtrT<CtrName>>::make_shared(this->shared_from_this(), CTR_FIND | CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create(const UUID& name)
    {
    	checkIfConainersCreationAllowed();
        return CtrMakeSharedPtr<CtrT<CtrName>>::make_shared(this->shared_from_this(), CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create()
    {
    	checkIfConainersCreationAllowed();
        auto ptr = CtrMakeSharedPtr<CtrT<CtrName>>::make_shared(this->shared_from_this(), CTR_CREATE, CTR_DEFAULT_NAME);
        return ptr;
    }

    template <typename CtrName>
    auto find(const UUID& name)
    {
    	checkIfConainersOpeneingAllowed();
        return CtrMakeSharedPtr<CtrT<CtrName>>::make_shared(this->shared_from_this(), CTR_FIND, name);
    }

    void pack_allocator()
    {
    	this->allocator_raw_->pack();
    }

protected:


    auto export_page_rchandle(const UUID& id)
    {
    	auto opt = persistent_tree_.find(id);

    	if (opt)
    	{
    		return opt.value().page_ptr();
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Page with id " << id << " does not exist in snapshot " << currentTxnId());
    	}
    }




    void clone_foreign_page(const Page* foreign_page)
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
        auto ptr = new RCPagePtr(page, 1, cpu_id_);
        auto old_value = persistent_tree_.assign(page->id(), Value(ptr, txn_id));

        if (old_value.page_ptr())
        {
        	if (old_value.page_ptr()->unref() == 0) {
        		delete old_value.page_ptr();
        	}
        }
    }

    void checkIfConainersOpeneingAllowed()
    {
    	checkReadAllowed();

    	if (is_data_locked())
    	{
    		throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " data is locked");
    	}
    }

    void checkIfConainersCreationAllowed()
    {
    	if (!is_active())
    	{
    		throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " data is not active");
    	}
    }


    void checkIfExportAllowed()
    {
    	if (history_node_->is_dropped())
    	{
    		// Double checking. This shouldn't happen
    		if (!history_node_->root())
    		{
    			throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " has been cleared");
    		}
    	}
    	else if (history_node_->is_active()) {
    		throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " is still active");
    	}
    }




    void checkReadAllowed()
    {
    	// read is always allowed
    }

    void checkUpdateAllowed()
    {
        checkReadAllowed();

        if (!history_node_->is_active())
        {
            throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " has been already committed or data is locked");
        }
    }

    void checkUpdateAllowed(const UUID& ctrName)
    {
    	checkReadAllowed();

    	if ((!history_node_->is_active()) && ctrName.is_set())
    	{
    		throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " has been already committed or data is locked");
    	}
    }

    void checkIfDataLocked()
    {
    	checkReadAllowed();

    	if (!history_node_->is_data_locked())
    	{
    		throw Exception(MA_SRC, SBuf() << "Snapshot " << uuid() << " hasn't been locked");
    	}
    }

    void do_drop_active() throw()
    {
    	allocator_raw_->unref_active();

    	allocator_raw_->store_mutex().read_lock().then([=]{
    		return do_drop().then([=]{
    			allocator_raw_->forget_snapshot(history_node_);
    			return ::now();
    		});
    	}).finally([=]{
    		allocator_raw_->store_mutex().read_unlock();
    	}).get0();
    }

    void do_drop_dropped() throw()
    {
    	allocator_raw_->unref_active();

    	allocator_raw_->store_mutex().read_lock().then([=]{
    		return do_drop().then([=]{
    			// FIXME
    			check_tree_structure(history_node_->root());
    			return ::now();
    		});
    	}).finally([=]{
        	allocator_raw_->store_mutex().read_unlock();
        }).get0();
    }


    future<> do_drop() throw ()
    {
    	return submit_async_to(cpu_id_, [this]{
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

						Int cpu_id = page_descr.page_ptr()->cpu_id();
						delete_on(cpu_id, page_descr.page_ptr()).get0();
					}
				}
			});
    	});
    }

    static void delete_snapshot(HistoryNode* node) throw ()
    {
    	//FIXME check if cpu_id is valid here
        PersitentTree persistent_tree(node, engine().cpu_id());

        persistent_tree.delete_tree([=](LeafNodeT* leaf){
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


    future<> check_tree_structure(const NodeBaseT* node)
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

    	return ::now();
    }

    template <typename Fn>
    auto submit_to(Int cpu_id, Fn&& fn) const
	{
    	return smp::submit_to(cpu_id, std::forward<Fn>(fn));
    }

    template <typename Fn>
    future<> submit_async_to(Int cpu_id, Fn&& fn) const
	{
    	return smp::submit_to(cpu_id, [&, fn = std::forward<Fn>(fn)]{
    		if (seastar::thread::running_in_thread()) {
    			fn();
    			return ::now();
    		}
    		else {
    			return seastar::async(fn);
    		}
    	});
	}

    void checkThreadAccess(const char* source, unsigned int line, const char* function) const
    {
    	Int cpu_id = engine().cpu_id();

    	if (cpu_id_ != cpu_id)
    	{
    		__assert_fail(
    				(SBuf()
    						<< "SeastarInMemSnapshot thread access assertion failed. Master thread: " << cpu_id_
							<< "Caller thread: " << cpu_id).str().c_str(),
					source,
					line,
					function
			);
    	}
    }
};

}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename Allocator>
auto create(const dumbo::shared_ptr<inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, Allocator>>& alloc, const UUID& name)
{
    return alloc->template create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename Allocator>
auto create(const dumbo::shared_ptr<inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, Allocator>>& alloc)
{
    std::cout << "Create Ctr: " << TypeNameFactory<CtrName>::name() << "\n";

	return alloc->template create<CtrName>();
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename Allocator>
auto find_or_create(const dumbo::shared_ptr<inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, Allocator>>& alloc, const UUID& name)
{
    return alloc->template find_or_create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PageType, typename HistoryNode, typename PersitentTree, typename Allocator>
auto find(const dumbo::shared_ptr<inmem::Snapshot<Profile, PageType, HistoryNode, PersitentTree, Allocator>>& alloc, const UUID& name)
{
    return alloc->template find<CtrName>(name);
}


template <typename Allocator>
void check_snapshot(const dumbo::shared_ptr<Allocator>& allocator, const char* message,  const char* source)
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
void check_snapshot(const dumbo::shared_ptr<Allocator>& allocator)
{
    check_snapshot(allocator, "Snapshot check failed", MA_RAW_SRC);
}



}}
