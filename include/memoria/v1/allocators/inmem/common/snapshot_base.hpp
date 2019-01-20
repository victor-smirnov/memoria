
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

#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/api/allocator/allocator_inmem_threads_api.hpp>
#include <memoria/v1/api/allocator/allocator_inmem_api.hpp>

#include <memoria/v1/allocators/inmem/common/allocator_stat.hpp>


#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/ctr_impl.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include "persistent_tree.hpp"

#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
namespace v1 {
namespace persistent_inmem {

template <typename Profile, typename PersistentAllocator, typename SnapshotType>
class SnapshotBase:
        public ProfileAllocatorType<Profile>,
        public SnpSharedFromThis<SnapshotType>
{    
protected:
	using MyType			= SnapshotType;
    using PageType          = ProfileBlockType<Profile>;
    using Base              = ProfileAllocatorType<Profile>;

public:
    using typename Base::CtrID;
    using typename Base::BlockID;
    using typename Base::BlockType;
    using typename Base::SnapshotID;
protected:

	using HistoryNode		= typename PersistentAllocator::HistoryNode;
    using PersistentTreeT   = typename PersistentAllocator::PersistentTreeT;
    


    using PersistentAllocatorPtr = AllocSharedPtr<PersistentAllocator>;
    using SnapshotPtr            = SnpSharedPtr<MyType>;
    using AllocatorPtr           = AllocSharedPtr<Base>;
    
    using NodeBaseT         = typename PersistentTreeT::NodeBaseT;
    using LeafNodeT         = typename PersistentTreeT::LeafNodeT;
    using PTreeValue        = typename LeafNodeT::Value;
    
    using RCPagePtr			= typename std::remove_pointer<typename PersistentTreeT::Value::Value>::type;

    using Status            = typename HistoryNode::Status;



    class CtrDescr {
    	int64_t references_;
    public:
    	CtrDescr(): references_() {}
    	CtrDescr(int64_t val): references_(val) {}

    	int64_t references() const {return references_;}
    	void ref() 	 {++references_;}
    	int64_t unref() {return --references_;}
    };

    using CtrInstanceMap 	= std::unordered_map<std::type_index, CtrDescr>;

public:

    template <typename CtrName>
    using CtrT = SharedCtr<CtrName, ProfileAllocatorType<Profile>, Profile>;

    template <typename CtrName>
    using CtrPtr = CtrSharedPtr<CtrT<CtrName>>;

    using typename Base::Page;
    using typename Base::PageG;
    using typename Base::Shared;

    using RootMapType = CtrT<Map<CtrID, BlockID>>;

protected:
    class Properties: public IAllocatorProperties {
    public:
        virtual int32_t defaultPageSize() const
        {
            return 8192;
        }

        virtual int64_t lastCommitId() const {
            return 0;
        }

        virtual void setLastCommitId(int64_t txn_id) {}

        virtual int64_t newTxnId() {return 0;}
    };

    HistoryNode*            history_node_;
    PersistentAllocatorPtr  history_tree_;
    PersistentAllocator*    history_tree_raw_ = nullptr;

    PersistentTreeT persistent_tree_;

    StaticPool<BlockID, Shared, 256>  pool_;

    Logger logger_;

    Properties properties_;

    CtrInstanceMap instance_map_;

    ContainerMetadataRepository* metadata_; //FIXME:: make it static thread local or remove at all

    template <typename>
    friend class ThreadInMemAllocatorImpl;
    
    template <typename>
    friend class InMemAllocatorImpl;
    
    
    template <typename, typename>
    friend class InMemAllocatorBase;
    
    
    template <typename>
    friend class memoria::v1::ThreadInMemSnapshot;
    
    template <typename>
    friend class memoria::v1::InMemSnapshot;

    PairPtr pair_;
    
    CtrSharedPtr<RootMapType> root_map_;

    int32_t ctr_op_;
    
public:

    SnapshotBase(HistoryNode* history_node, const PersistentAllocatorPtr& history_tree):
        history_node_(history_node),
        history_tree_(history_tree),
        history_tree_raw_(history_tree.get()),
        persistent_tree_(history_node_),
        logger_("PersistentInMemAllocatorSnp", Logger::DERIVED, &history_tree->logger_),
        metadata_(MetadataRepository<Profile>::getMetadata())
    {
        //root_map_->getMetadata();
        
    	history_node_->ref();

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
            ctr_op_ = CTR_CREATE | CTR_FIND;
        }
        else {
            ctr_op_ = CTR_FIND;
        }
    }

    SnapshotBase(HistoryNode* history_node, PersistentAllocator* history_tree):
        history_node_(history_node),
        history_tree_raw_(history_tree),
        persistent_tree_(history_node_),
        logger_("PersistentInMemAllocatorTxn"),
        metadata_(MetadataRepository<Profile>::getMetadata())
    {
        //root_map_->getMetadata();
        
    	history_node_->ref();

        if (history_node->is_active())
        {
            history_tree_raw_->ref_active();
            ctr_op_ = CTR_CREATE | CTR_FIND;
        }
        else {
            ctr_op_ = CTR_FIND;
        }
    }
    
    void post_init() 
    {
        auto ptr = this->shared_from_this();
        root_map_ = ctr_make_shared<RootMapType>(ptr, ctr_op_, CtrID{});
        root_map_->reset_allocator_holder();
    }
    
    static void initMetadata() {
        RootMapType::getMetadata();
    }


    virtual ~SnapshotBase()
    {
    }
    
    virtual SnpSharedPtr<ProfileAllocatorType<Profile>> self_ptr() {
        return this->shared_from_this();
    }
    
    PairPtr& pair() {
        return pair_;
    }

    const PairPtr& pair() const {
        return pair_;
    }

    ContainerMetadataRepository* getMetadata() const {
        return metadata_;
    }

    const auto& uuid() const {
        return history_node_->snapshot_id();
    }

    bool is_active() const {
        return history_node_->is_active();
    }

    bool is_data_locked() const {
    	return history_node_->is_data_locked();
    }

    virtual bool isActive() const {
        return is_active();
    }

    bool is_marked_to_clear() const {
        return history_node_->is_dropped();
    }

    bool is_committed() const {
        return history_node_->is_committed();
    }

    std::vector<CtrID> container_names() const
    {
        std::vector<CtrID> names;

        auto ii = root_map_->begin();
        while (!ii->isEnd()) {
            names.push_back(ii->key());
            ii->next();
        }

        return names;
    }

    std::vector<U16String> container_names_str() const
    {
        std::vector<U16String> names;

        auto ii = root_map_->begin();
        while (!ii->isEnd()) {
            names.push_back(ii->key().to_u16());
            ii->next();
        }

        return names;
    }


    bool drop_ctr(const CtrID& name)
    {
        checkUpdateAllowed();

        CtrID root_id = getRootID(name);

        if (root_id.is_set())
        {
            PageG page = this->getBlock(root_id);

            auto& ctr_meta = getMetadata()->getContainerMetadata(page->ctr_type_hash());

            ctr_meta->getCtrInterface()->drop(root_id, name, this->shared_from_this());

            return true;
        }
        else {
            return false;
        }
    }


    Optional<U16String> ctr_type_name_for(const CtrID& name)
    {
        auto root_id = this->getRootID(name);
        auto page 	 = this->getBlock(root_id);

        if (page)
        {
            auto ctr_hash = page->ctr_type_hash();
            auto ctr_meta = metadata_->getContainerMetadata(ctr_hash);

            return ctr_meta->getCtrInterface()->ctr_type_name();
        }
        else {
            return Optional<U16String>();
        }
    }

    void set_as_master()
    {
    	history_tree_raw_->set_master(uuid());
    }

    void set_as_branch(U16StringRef name)
    {
    	history_tree_raw_->set_branch(name, uuid());
    }

    U16StringRef metadata() const
    {
        return history_node_->metadata();
    }

    void set_metadata(U16StringRef metadata)
    {
        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            MMA1_THROW(Exception()) << WhatCInfo("Snapshot is already committed.");
        }
    }

    void for_each_ctr_node(const CtrID& name, typename ContainerInterface::BlockCallbackFn fn)
    {
    	auto root_id = this->getRootID(name);
        auto page 	 = this->getBlock(root_id);

    	if (page)
    	{
    		auto ctr_hash   = page->ctr_type_hash();
    		auto ctr_meta   = metadata_->getContainerMetadata(ctr_hash);

    		ctr_meta->getCtrInterface()->for_each_ctr_node(name, this->shared_from_this(), fn);
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container with name {} does not exist in snapshot {}", name, history_node_->snapshot_id()));
    	}
    }



    void import_new_ctr_from(const SnapshotPtr& txn, const CtrID& name)
    {
        checkUpdateAllowed();
    	txn->checkIfExportAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockID& uuid, const BlockID& id, const void* page_data){
    			auto rc_handle = txn->export_page_rchandle(id);
    			using Value = typename PersistentTreeT::Value;

    			rc_handle->ref();

    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

    			if (old_value.page_ptr())
    			{
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Page with ID {} is not new in snapshot {}", id, txn_id));
    			}
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()));
    		}
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container with name {} already exists in snapshot {}", name, txn_id));
    	}
    }


    void copy_new_ctr_from(const SnapshotPtr& txn, const CtrID& name)
    {
    	txn->checkReadAllowed();
    	checkUpdateAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&](const BlockID& uuid, const BlockID& id, const void* page_data){
    			clone_foreign_page(T2T<const Page*>(page_data));
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()));
    		}
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container with name {} already exists in snapshot {}", name, txn_id));
    	}
    }


    void import_ctr_from(const SnapshotPtr& txn, const CtrID& name)
    {
        checkUpdateAllowed();
    	txn->checkIfExportAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = uuid();

    	if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockID& uuid, const BlockID& id, const void* page_data) {
                auto page = this->getBlock(id);
    			if (page && page->uuid() == uuid)
    			{
    				return;
    			}

    			auto rc_handle = txn->export_page_rchandle(id);
    			using Value = typename PersistentTreeT::Value;

    			rc_handle->ref();

    			auto old_value = persistent_tree_.assign(id, Value(rc_handle, txn_id));

    			if (old_value.page_ptr())
    			{
    				if (old_value.page_ptr()->unref() == 0)
    				{
    					// FIXME: just delete the page?
                        MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected refcount == 0 for page {}", old_value.page_ptr()->raw_data()->uuid()));
    				}
    			}
    		});

    		auto root_id = txn->getRootID(name);
    		if (root_id.is_set())
    		{
    			root_map_->assign(name, root_id);
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn->currentTxnId()));
    		}
    	}
    	else {
    		import_new_ctr_from(txn, name);
    	}
    }


    void copy_ctr_from(const SnapshotPtr& txn, const CtrID& name)
    {
    	txn->checkReadAllowed();
    	checkUpdateAllowed();

        BlockID root_id = this->getRootID(name);

    	auto txn_id = currentTxnId();

    	if (!root_id.is_null())
    	{
            txn->for_each_ctr_node(name, [&, this](const BlockID& uuid, const BlockID& id, const void* page_data) {
                auto page = this->getBlock(id);
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
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unexpected empty root ID for container {} in snapshot {}", name, txn_id));
    		}
    	}
    	else {
    		copy_new_ctr_from(txn, name);
    	}
    }


    CtrID clone_ctr(const CtrID& ctr_name) {
        return clone_ctr(ctr_name, CtrID{});
    }

    CtrID clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name)
    {
        auto root_id = this->getRootID(ctr_name);
        auto page 	 = this->getBlock(root_id);

        if (page)
        {
            auto ctr_hash    = page->ctr_type_hash();
            auto ctr_meta   = metadata_->getContainerMetadata(ctr_hash);

            return ctr_meta->getCtrInterface()->clone_ctr(ctr_name, new_ctr_name, this->shared_from_this());
        }
        else {
            MMA1_THROW(Exception()) << fmt::format_ex(u"Container with name {} does not exist in snapshot {} ", ctr_name, history_node_->snapshot_id());
        }
    }


    virtual PageG getBlock(const BlockID& id)
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
                    const auto& txn_id = history_node_->snapshot_id();

                    if (page_opt.value().snapshot_id() != txn_id)
                    {
                        shared->state() = Shared::READ;
                    }
                    else {
                        shared->state() = Shared::UPDATE;
                    }

                    shared->set_page(page_opt.value().page_ptr()->raw_data());
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Page is not found for the specified id: {}", id));
                }
            }

            return PageG(shared);
        }
        else {
            return PageG();
        }
    }

    void dumpAccess(const char* msg, const BlockID& id, const Shared* shared)
    {
        std::cout << msg << ": " << id << " " << shared->get() << " " << shared->get()->uuid() << " " << shared->state() << std::endl;
    }


    virtual void registerCtr(const std::type_info& ti)
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

    virtual void unregisterCtr(const std::type_info& ti)
    {
    	auto ii = instance_map_.find(ti);
    	if (ii == instance_map_.end())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Container {} is not registered in snapshot {}", ti.name(), uuid()));
    	}
    	else if (ii->second.unref() == 0) {
    		instance_map_.erase(ii);
    	}
    }

public:
    bool has_open_containers() {
    	return instance_map_.size() > 1;
    }

    void dump_open_containers()
    {
    	for (const auto& pair: instance_map_)
    	{
            std::cout << demangle(pair.first.name()) << " -- " << pair.second.references() << std::endl;
    	}
    }

    void dump_dictionary_pages()
    {
        auto ii = root_map_->begin();
        if (!ii->isEnd())
        {
            do {
                ii->dump();
            }
            while (ii->nextLeaf());
        }
    }


    virtual PageG getBlockForUpdate(const BlockID& id)
    {
        // FIXME: Though this check prohibits new page acquiring for update,
        // already acquired updatable pages can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(CtrID{});

        if (id.isSet())
        {
            Shared* shared = get_shared(id, Shared::UPDATE);

            if (!shared->get())
            {
                auto page_opt = persistent_tree_.find(id);

                if (page_opt)
                {
                    const auto& txn_id = history_node_->snapshot_id();

                    if (page_opt.value().snapshot_id() != txn_id)
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
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Page is not found for the specified id: {}", id));
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
                    MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Page is not found for the specified id: {}", id));
                }
            }
            else if (shared->state() == Shared::UPDATE)
            {
                //MEMORIA_ASEERT();
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid PageShared state: {}", shared->state()));
            }

            shared->state() = Shared::UPDATE;

            return PageG(shared);
        }
        else {
            return PageG();
        }
    }



    virtual PageG updateBlock(Shared* shared)
    {
        // FIXME: Though this check prohibits new page acquiring for update,
        // already acquired updatable pages can be updated further.
        // To guarantee non-updatability, MMU-protection should be used
        checkUpdateAllowed(CtrID{});

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

    virtual void removeBlock(const BlockID& id)
    {
        checkUpdateAllowed(CtrID{});

        auto iter = persistent_tree_.locate(id);

        if (!iter.is_end())
        {
            auto shared = pool_.get(id);

            if (!shared)
            {
                persistent_tree_.remove(iter);
            }
            else {
                shared->state() = Shared::_DELETE;
            }
        }
    }






    virtual PageG createBlock(int32_t initial_size)
    {
        checkUpdateAllowed(CtrID{});

        if (initial_size == -1)
        {
            initial_size = properties_.defaultPageSize();
        }

        void* buf = allocate_system<void>(initial_size).release();

        memset(buf, 0, initial_size);

        BlockID id = newId();

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


    virtual PageG cloneBlock(const Shared* shared, const BlockID& new_id)
    {
        checkUpdateAllowed(CtrID{});

        BlockID new_id_v = new_id.is_set() ? new_id : newId();
        Page* new_page = this->clone_page(shared->get());

        new_page->id() = new_id_v;

        Shared* new_shared  = pool_.allocate(new_id_v);

        new_shared->id()    = new_id_v;
        new_shared->state() = Shared::UPDATE;

        new_shared->set_page(new_page);
        new_shared->set_allocator(this);

        ptree_set_new_page(new_page);

        return PageG(new_shared);
    }


    virtual void resizeBlock(Shared* shared, int32_t new_size)
    {
        checkUpdateAllowed();

        if (shared->state() == Shared::READ)
        {
            Page* page = shared->get();
            auto pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

            Page* new_page = allocate_system<Page>(new_size).release();

            pageMetadata->getPageOperations()->resize(page, new_page, new_size);

            shared->set_page(new_page);

            ptree_set_new_page(new_page);
        }
        else if (shared->state() == Shared::UPDATE)
        {
            Page* page = shared->get();
            auto pageMetadata = metadata_->getPageMetadata(page->ctr_type_hash(), page->page_type_hash());

            Page* new_page = reallocate_system<Page>(page, new_size).release();

            pageMetadata->getPageOperations()->resize(page, new_page, new_size);

            shared->set_page(new_page);

            ptree_set_new_page(new_page);
        }
    }

    virtual void releaseBlock(Shared* shared) noexcept
    {
        if (shared->state() == Shared::_DELETE)
        {
            persistent_tree_.remove(shared->get()->id());
        }

        pool_.release(shared->id());
    }

    virtual PageG getBlockG(Page* page)
    {
        MMA1_THROW(Exception()) << WhatCInfo("Method getBlockG is not implemented for this allocator");
    }


    virtual BlockID newId() {
        return history_tree_raw_->newId();
    }

    virtual CtrID currentTxnId() const {
        return history_node_->snapshot_id();
    }

    // memory pool allocator
    virtual void* allocateMemory(size_t size) {
        return allocate_system<void>(size).release();
    }
    virtual void  freeMemory(void* ptr) {
        free_system(ptr);
    }

    virtual Logger& logger() {return logger_;}
    virtual IAllocatorProperties& allocator_properties() {
        return properties_;
    }

    virtual BlockID getRootID(const CtrID& name)
    {
        if (!name.is_null())
        {
            auto iter = root_map_->find(name);

            if (iter->is_found(name))
            {
                return iter->value();
            }
            else {
                return BlockID{};
            }
        }
        else {
            return history_node_->root_id();
        }
    }

    virtual void setRoot(const CtrID& name, const BlockID& root)
    {
        if (root.is_null())
        {
            if (!name.is_null())
            {
                root_map_->remove(name);
            }
            else {
                MMA1_THROW(Exception()) << WhatCInfo("Allocator directory removal attempted");
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

    virtual bool hasRoot(const CtrID& name)
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

    virtual CtrID createCtrName()
    {
        return IDTools<CtrID>::make_random();
    }


    virtual bool check()
    {
        bool result = false;

        for (auto iter = root_map_->begin(); !iter->is_end(); )
        {
            auto ctr_name = iter->key();

            PageG page = this->getBlock(iter->value());

            auto ctr_meta = metadata_->getContainerMetadata(page->ctr_type_hash());

            result = ctr_meta->getCtrInterface()->check(page->id(), ctr_name, this->shared_from_this()) || result;

            iter->next();
        }

        return result;
    }

    U16String get_branch_suffix() const
    {
        return u"";
    }

    virtual void walkContainers(ContainerWalker* walker, const char16_t* allocator_descr = nullptr)
    {
		if (allocator_descr != nullptr)
		{
            walker->beginSnapshot(fmt::format(u"Snapshot-{} -- {}", history_node_->snapshot_id(), allocator_descr).data());
		}
		else {
            walker->beginSnapshot(fmt::format(u"Snapshot-{}", history_node_->snapshot_id()).data());
		}

        auto iter = root_map_->Begin();

        while (!iter->isEnd())
        {
            auto ctr_name   = iter->key();
            auto root_id    = iter->value();

            auto page       = this->getBlock(root_id);

            auto ctr_hash   = page->ctr_type_hash();
            auto ctr_meta   = metadata_->getContainerMetadata(ctr_hash);

            ctr_meta->getCtrInterface()->walk(page->id(), ctr_name, this->shared_from_this(), walker);

            iter->next();
        }

        walker->endSnapshot();
    }



    void dump_persistent_tree() {
        persistent_tree_.dump_tree();
    }



    template <typename CtrName>
    auto find_or_create(const CtrID& name)
    {
    	checkIfConainersCreationAllowed();
        return ctr_make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_FIND | CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create(const CtrID& name)
    {
    	checkIfConainersCreationAllowed();
        return ctr_make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_CREATE, name);
    }

    template <typename CtrName>
    auto create()
    {
    	checkIfConainersCreationAllowed();
        return ctr_make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_CREATE, CTR_DEFAULT_NAME);
    }

    template <typename CtrName>
    auto find(const CtrID& name)
    {
    	checkIfConainersOpeneingAllowed();
        return ctr_make_shared<CtrT<CtrName>>(this->shared_from_this(), CTR_FIND, name);
    }

    void pack_allocator()
    {
    	this->history_tree_raw_->pack();
    }
    
    virtual CtrSharedPtr<CtrReferenceable> get(const CtrID& name)
    {
        CtrID root_id = getRootID(name);

        if (root_id.is_set())
        {
            PageG page = this->getBlock(root_id);

            auto& ctr_meta = getMetadata()->getContainerMetadata(page->ctr_type_hash());

            return ctr_meta->getCtrInterface()->new_ctr_instance(root_id, name, this->shared_from_this());
        }
        else {
            return CtrSharedPtr<CtrReferenceable>();
        }
    }

    virtual U16String ctr_type_name(const CtrID& name)
    {
        CtrID root_id = getRootID(name);

        if (root_id.is_set())
        {
            PageG page = this->getBlock(root_id);

            auto& ctr_meta = getMetadata()->getContainerMetadata(page->ctr_type_hash());

            return ctr_meta->getCtrInterface()->ctr_type_name();
        }
        else {
            MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't find container with name {}", name);
        }
    }

    virtual CtrSharedPtr<CtrReferenceable> from_root_id(const CtrID& root_page_id, const CtrID& name)
    {
        if (root_page_id.is_set())
        {
            PageG page = this->getBlock(root_page_id);

            auto& ctr_meta = getMetadata()->getContainerMetadata(page->ctr_type_hash());

            return ctr_meta->getCtrInterface()->new_ctr_instance(root_page_id, name, this->shared_from_this());
        }
        else {
            return CtrSharedPtr<CtrReferenceable>();
        }
    }

    CtrPageDescription describe_page(const CtrID& page_id)
    {
        PageG page = this->getBlock(page_id);
        return describe_page(page);
    }

    CtrPageDescription describe_page(const PageG& page)
    {
        auto& ctr_meta = getMetadata()->getContainerMetadata(page->ctr_type_hash());
        return ctr_meta->getCtrInterface()->describe_page(page->id(), this->shared_from_this());
    }

protected:

    SharedPtr<SnapshotMemoryStat> do_compute_memory_stat(bool include_containers)
    {
        _::PageSet visited_pages;

        PersistentTreeStatVisitAccumulatingConsumer vp_accum(visited_pages);

        HistoryNode* node = this->history_node_->parent();

        while (node)
        {
            PersistentTreeT persistent_tree(node);
            persistent_tree.conditional_tree_traverse(vp_accum);

            node = node->parent();
        }

        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_pages, this, include_containers);

        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    SharedPtr<SnapshotMemoryStat> do_compute_memory_stat(_::PageSet& visited_pages, bool include_containers)
    {
        SnapshotStatsCountingConsumer<SnapshotBase> consumer(visited_pages, this, include_containers);

        persistent_tree_.conditional_tree_traverse(consumer);

        return consumer.finish();
    }

    auto export_page_rchandle(const CtrID& id)
    {
    	auto opt = persistent_tree_.find(id);

    	if (opt)
    	{
    		return opt.value().page_ptr();
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Page with id {} does not exist in snapshot {}", id, currentTxnId()));
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

    Shared* get_shared(const BlockID& id, int32_t state)
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
        const auto& txn_id = history_node_->snapshot_id();
        using Value = typename PersistentTreeT::Value;
        auto ptr = new RCPagePtr(page, 1);
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot's {} data is locked", uuid()));
    	}
    }

    void checkIfConainersCreationAllowed()
    {
    	if (!is_active())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot's {} data is not active, snapshot status = {}", uuid(), (int32_t)history_node_->status()));
    	}
    }


    void checkIfExportAllowed()
    {
    	if (history_node_->is_dropped())
    	{
    		// Double checking. This shouldn't happen
    		if (!history_node_->root())
    		{
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has been cleared", uuid()));
    		}
    	}
    	else if (history_node_->is_active()) {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is still active", uuid()));
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
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has been already committed or data is locked", uuid()));
        }
    }

    void checkUpdateAllowed(const CtrID& ctrName)
    {
    	checkReadAllowed();

    	if ((!history_node_->is_active()) && ctrName.is_set())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has been already committed or data is locked", uuid()));
    	}
    }

    void checkIfDataLocked()
    {
    	checkReadAllowed();

    	if (!history_node_->is_data_locked())
    	{
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} hasn't been locked", uuid()));
    	}
    }


    void do_drop() throw ()
    {
        if (history_tree_raw_->isDumpSnapshotLifecycle()) {
            std::cout << "MEMORIA: DROP snapshot's DATA: " << history_node_->snapshot_id() << std::endl;
        }

    	persistent_tree_.delete_tree([&](LeafNodeT* leaf){
            for (int32_t c = 0; c < leaf->size(); c++)
            {
                auto& page_descr = leaf->data(c);
                if (page_descr.page_ptr()->unref() == 0)
                {
                    auto shared = pool_.get(page_descr.page_ptr()->raw_data()->id());

                    if (shared)
                    {
                    	page_descr.page_ptr()->clear();
                        shared->state() = Shared::_DELETE;
                    }

                    delete page_descr.page_ptr();
                }
            }
        });
    }

    static void delete_snapshot(HistoryNode* node) throw ()
    {
        PersistentTreeT persistent_tree(node);

        persistent_tree.delete_tree([&](LeafNodeT* leaf){
            for (int32_t c = 0; c < leaf->size(); c++)
            {
                auto& page_descr = leaf->data(c);
                if (page_descr.page_ptr()->unref() == 0)
                {
                    delete page_descr.page_ptr();
                }
            }
        });

        node->assign_root_no_ref(nullptr);
        node->root_id() = BlockID{};
    }



    void check_tree_structure(const NodeBaseT* node)
    {
//      if (node->snapshot_id() == history_node_->snapshot_id())
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
//          for (int32_t c = 0; c < branch_node->size(); c++)
//          {
//              check_tree_structure(branch_node->data(c));
//          }
//      }
    }
};

}}}
