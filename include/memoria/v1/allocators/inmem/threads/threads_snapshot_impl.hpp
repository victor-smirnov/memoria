
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


#include "../common/snapshot_base.hpp"

#include <memoria/v1/api/allocator/allocator_inmem_threads_api.hpp>

#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/ctr_impl.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/type_name.hpp>



#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
namespace v1 {
namespace persistent_inmem {



template <typename Profile, typename PersistentAllocator>
class ThreadSnapshot: public SnapshotBase<Profile, PersistentAllocator, ThreadSnapshot<Profile, PersistentAllocator>>
{    
protected:
    using Base = SnapshotBase<Profile, PersistentAllocator, ThreadSnapshot<Profile, PersistentAllocator>>;
    using MyType = ThreadSnapshot<Profile, PersistentAllocator>;
    
    using typename Base::PersistentAllocatorPtr;
    using typename Base::HistoryNode;
    using typename Base::SnapshotPtr;

    using AllocatorMutexT	= typename std::remove_reference<decltype(std::declval<HistoryNode>().allocator_mutex())>::type;
    using MutexT			= typename std::remove_reference<decltype(std::declval<HistoryNode>().snapshot_mutex())>::type;
    using StoreMutexT		= typename std::remove_reference<decltype(std::declval<HistoryNode>().store_mutex())>::type;

    using LockGuardT			= typename std::lock_guard<MutexT>;
    using StoreLockGuardT		= typename std::lock_guard<StoreMutexT>;
    using AllocatorLockGuardT	= typename std::lock_guard<AllocatorMutexT>;
    
    using typename Base::BlockID;
    using typename Base::CtrID;
    using typename Base::BlockType;
    using typename Base::SnapshotID;
    
    using Base::history_node_;
    using Base::history_tree_;
    using Base::history_tree_raw_;
    using Base::do_drop;
    using Base::check_tree_structure;


public:
    using Base::has_open_containers;


    using Base::uuid;
    
    
    ThreadSnapshot(HistoryNode* history_node, const PersistentAllocatorPtr& history_tree):
        Base(history_node, history_tree)
    {}

    ThreadSnapshot(HistoryNode* history_node, PersistentAllocator* history_tree):
        Base(history_node, history_tree)
    {
    }
 
    virtual ~ThreadSnapshot()
    {
    	//FIXME This code doesn't decrement properly number of active snapshots
    	// for allocator to store data correctly.

    	bool drop1 = false;
    	bool drop2 = false;

    	{
    		LockGuardT snapshot_lock_guard(history_node_->snapshot_mutex());

    		if (history_node_->unref() == 0)
    		{
    			if (history_node_->is_active())
    			{
    				drop1 = true;
                    history_tree_raw_->unref_active();
    			}
    			else if(history_node_->is_dropped())
    			{
    				drop2 = true;
    				check_tree_structure(history_node_->root());
    			}
    		}
    	}

    	if (drop1)
    	{
    		do_drop();
    		history_tree_raw_->forget_snapshot(history_node_);
    	}

    	if (drop2)
    	{
    		StoreLockGuardT store_lock_guard(history_node_->store_mutex());
    		do_drop();

    		// FIXME: check if absence of snapshot lock here leads to data races...
    	}
    }

    SnapshotMetadata<SnapshotID> describe() const
    {
    	std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        std::vector<SnapshotID> children;

    	for (const auto& node: history_node_->children())
    	{
            children.emplace_back(node->snapshot_id());
    	}

        auto parent_id = history_node_->parent() ? history_node_->parent()->snapshot_id() : SnapshotID{};

        return SnapshotMetadata<SnapshotID>(parent_id, history_node_->snapshot_id(), children, history_node_->metadata(), history_node_->status());
    }

    void commit()
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active() || history_node_->is_data_locked())
        {
            history_node_->commit();
            history_tree_raw_->unref_active();

            if (history_tree_raw_->isDumpSnapshotLifecycle()) {
                std::cout << "MEMORIA: COMMIT snapshot: " << history_node_->snapshot_id() << std::endl;
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid()));
        }
    }

    void drop()
    {
    	std::lock(history_node_->allocator_mutex(), history_node_->snapshot_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->parent() != nullptr)
        {
            if (history_node_->is_active())
            {
                history_tree_raw_->unref_active();
            }

            if (history_node_->status() == SnapshotStatus::COMMITTED)
            {
                history_tree_raw_->adjust_named_references(history_node_);
            }

            history_node_->mark_to_clear();

            if (history_tree_raw_->isDumpSnapshotLifecycle()) {
                std::cout << "MEMORIA: MARK snapshot DROPPED: " << history_node_->snapshot_id() << std::endl;
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Can't drop root snapshot {}", uuid()));
        }
    }

    U16String metadata() const
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());
        return history_node_->metadata();
    }

    void set_metadata(U16StringRef metadata)
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            MMA1_THROW(Exception()) << WhatCInfo("Snapshot is already committed.");
        }
    }

    void lock_data_for_import()
    {
    	std::lock(history_node_->allocator_mutex(), history_node_->snapshot_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

    	if (history_node_->is_active())
    	{
    		if (!has_open_containers())
    		{
    			history_node_->lock_data();
    		}
    		else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has open containers", uuid()));
    		}
    	}
    	else if (history_node_->is_data_locked()) {
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid()));
    	}
    }


    SnapshotPtr branch()
    {
    	std::lock(history_node_->allocator_mutex(), history_node_->snapshot_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->is_committed())
        {
            HistoryNode* history_node = new HistoryNode(history_node_);

            if (history_tree_raw_->isDumpSnapshotLifecycle()) {
                std::cout << "MEMORIA: BRANCH snapshot: " << history_node->snapshot_id() << std::endl;
            }

            LockGuardT lock_guard3(history_node->snapshot_mutex());

            history_tree_raw_->snapshot_map_[history_node->snapshot_id()] = history_node;

            return snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this());
        }
        else if (history_node_->is_data_locked())
        {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is locked, branching is not possible.", uuid()));
        }
        else
        {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} is still being active. Commit it first.", uuid()));
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
            return snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this());
        }
        else
        {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has no parent.", uuid()));
        }
    }

    SharedPtr<SnapshotMemoryStat> compute_memory_stat(bool include_containers)
    {
        std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());
        return this->do_compute_memory_stat(include_containers);
    }
};

}

template <typename CtrName, typename Profile, typename PersistentAllocator>
auto create(const SnpSharedPtr<persistent_inmem::ThreadSnapshot<Profile, PersistentAllocator>>& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PersistentAllocator>
auto create(const SnpSharedPtr<persistent_inmem::ThreadSnapshot<Profile, PersistentAllocator>>& alloc)
{
    return alloc->template create<CtrName>();
}

template <typename CtrName, typename Profile, typename PersistentAllocator>
auto find_or_create(const SnpSharedPtr<persistent_inmem::ThreadSnapshot<Profile, PersistentAllocator>>& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template find_or_create<CtrName>(name);
}

template <typename CtrName, typename Profile, typename PersistentAllocator>
auto find(const SnpSharedPtr<persistent_inmem::ThreadSnapshot<Profile, PersistentAllocator>>& alloc, const ProfileCtrID<Profile>& name)
{
    return alloc->template find<CtrName>(name);
}


template <typename Allocator>
void check_snapshot(Allocator& allocator, const char* message, const char* source)
{
    int32_t level = allocator->logger().level();

    allocator->logger().level() = Logger::_ERROR;

    if (allocator->check())
    {
        allocator->logger().level() = level;
        MMA1_THROW(Exception()) << WhatCInfo(message);
    }

    allocator->logger().level() = level;
}

template <typename Allocator>
void check_snapshot(Allocator& allocator)
{
    check_snapshot(allocator, "Snapshot check failed", MA_RAW_SRC);
}






template <typename Profile>
ThreadInMemSnapshot<Profile>::ThreadInMemSnapshot() {}


template <typename Profile>
ThreadInMemSnapshot<Profile>::ThreadInMemSnapshot(SnpSharedPtr<PImpl> impl): pimpl_(impl) {}

template <typename Profile>
ThreadInMemSnapshot<Profile>::ThreadInMemSnapshot(ThreadInMemSnapshot<Profile>&& other): pimpl_(std::move(other.pimpl_)) {}

template <typename Profile>
ThreadInMemSnapshot<Profile>::ThreadInMemSnapshot(const ThreadInMemSnapshot<Profile>&other): pimpl_(other.pimpl_) {}

template <typename Profile>
ThreadInMemSnapshot<Profile>& ThreadInMemSnapshot<Profile>::operator=(const ThreadInMemSnapshot<Profile>& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}

template <typename Profile>
ThreadInMemSnapshot<Profile>& ThreadInMemSnapshot<Profile>::operator=(ThreadInMemSnapshot<Profile>&& other) 
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}


template <typename Profile>
ThreadInMemSnapshot<Profile>::~ThreadInMemSnapshot(){}


template <typename Profile>
bool ThreadInMemSnapshot<Profile>::operator==(const ThreadInMemSnapshot& other) const
{
    return pimpl_ == other.pimpl_;
}

template <typename Profile>
ThreadInMemSnapshot<Profile>::operator bool() const
{
    return pimpl_ != nullptr; 
}




template <typename Profile>
const typename ThreadInMemSnapshot<Profile>::SnapshotID& ThreadInMemSnapshot<Profile>::uuid() const
{
    return pimpl_->uuid();
}

template <typename Profile>
bool ThreadInMemSnapshot<Profile>::is_active() const 
{
    return pimpl_->is_active();
}

template <typename Profile>
bool ThreadInMemSnapshot<Profile>::is_marked_to_clear() const 
{
    return pimpl_->is_marked_to_clear();
}

template <typename Profile>
bool ThreadInMemSnapshot<Profile>::is_committed() const 
{
    return pimpl_->is_committed();
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::commit() 
{
    return pimpl_->commit();
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::drop() 
{
    return pimpl_->drop();
}

template <typename Profile>
bool ThreadInMemSnapshot<Profile>::drop_ctr(const CtrID& name)
{
    return pimpl_->drop_ctr(name);
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::set_as_master() 
{
    return pimpl_->set_as_master();
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::set_as_branch(U16StringRef name)
{
    return pimpl_->set_as_branch(name);
}

template <typename Profile>
U16String ThreadInMemSnapshot<Profile>::snapshot_metadata() const
{
    return pimpl_->metadata();
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::set_snapshot_metadata(U16StringRef metadata)
{
    return pimpl_->set_metadata(metadata);
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::lock_data_for_import() 
{
    return pimpl_->lock_data_for_import();
}

template <typename Profile>
typename ThreadInMemSnapshot<Profile>::SnapshotPtr ThreadInMemSnapshot<Profile>::branch() 
{
    return pimpl_->branch();
}

template <typename Profile>
bool ThreadInMemSnapshot<Profile>::has_parent() const 
{
    return pimpl_->has_parent();
}

template <typename Profile>
typename ThreadInMemSnapshot<Profile>::SnapshotPtr ThreadInMemSnapshot<Profile>::parent() 
{
    return pimpl_->parent();
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::import_new_ctr_from(ThreadInMemSnapshot<Profile>& txn, const CtrID& name)
{
    return pimpl_->import_new_ctr_from(txn.pimpl_, name);
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::copy_new_ctr_from(ThreadInMemSnapshot<Profile>& txn, const CtrID& name)
{
    return pimpl_->copy_new_ctr_from(txn.pimpl_, name);
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::import_ctr_from(ThreadInMemSnapshot<Profile>& txn, const CtrID& name)
{
    return pimpl_->import_ctr_from(txn.pimpl_, name);
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::copy_ctr_from(ThreadInMemSnapshot<Profile>& txn, const CtrID& name)
{
    return pimpl_->copy_ctr_from(txn.pimpl_, name);
}

template <typename Profile>
bool ThreadInMemSnapshot<Profile>::check() {
    return pimpl_->check();
}

template <typename Profile>
typename ThreadInMemSnapshot<Profile>::CtrID ThreadInMemSnapshot<Profile>::clone_ctr(const CtrID& name, const CtrID& new_name) {
    return pimpl_->clone_ctr(name, new_name);
}

template <typename Profile>
typename ThreadInMemSnapshot<Profile>::CtrID ThreadInMemSnapshot<Profile>::clone_ctr(const CtrID& name) {
    return pimpl_->clone_ctr(name);
}


template <typename Profile>
void ThreadInMemSnapshot<Profile>::dump_persistent_tree() 
{
    return pimpl_->dump_persistent_tree();
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::dump_dictionary_blocks()
{
    return pimpl_->dump_dictionary_blocks();
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::dump_open_containers()
{
    return pimpl_->dump_open_containers();
}

template <typename Profile>
bool ThreadInMemSnapshot<Profile>::has_open_containers()
{
    return pimpl_->has_open_containers();
}

template <typename Profile>
Optional<U16String> ThreadInMemSnapshot<Profile>::ctr_type_name_for(const CtrID& name)
{
    return pimpl_->ctr_type_name_for(name);
}



template <typename Profile>
void ThreadInMemSnapshot<Profile>::walk_containers(ContainerWalker<Profile>* walker, const char16_t* allocator_descr)
{
     return pimpl_->walkContainers(walker, allocator_descr);
}

template <typename Profile>
void ThreadInMemSnapshot<Profile>::reset() 
{
    return pimpl_.reset();
}


template <typename Profile>
SnpSharedPtr<ProfileAllocatorType<Profile>> ThreadInMemSnapshot<Profile>::snapshot_ref_creation_allowed()
{
    pimpl_->checkIfConainersCreationAllowed();
    return memoria_static_pointer_cast<ProfileAllocatorType<Profile>>(pimpl_->shared_from_this());
}


template <typename Profile>
SnpSharedPtr<ProfileAllocatorType<Profile>> ThreadInMemSnapshot<Profile>::snapshot_ref_opening_allowed()
{
    pimpl_->checkIfConainersOpeneingAllowed();
    return memoria_static_pointer_cast<ProfileAllocatorType<Profile>>(pimpl_->shared_from_this());
}


template <typename Profile>
Logger& ThreadInMemSnapshot<Profile>::logger()
{
    return pimpl_->logger();
}


template <typename Profile>
CtrRef<Profile> ThreadInMemSnapshot<Profile>::get(const CtrID& name)
{
    return CtrRef<Profile>(pimpl_->get(name));
}


template <typename Profile>
const PairPtr& ThreadInMemSnapshot<Profile>::pair() const {
    return pimpl_->pair();
}

template <typename Profile>
PairPtr& ThreadInMemSnapshot<Profile>::pair() {
    return pimpl_->pair();
}

template <typename Profile>
std::vector<typename ThreadInMemSnapshot<Profile>::CtrID> ThreadInMemSnapshot<Profile>::container_names() const {
    return pimpl_->container_names();
}

template <typename Profile>
std::vector<U16String> ThreadInMemSnapshot<Profile>::container_names_str() const {
    return pimpl_->container_names_str();
}

template <typename Profile>
SharedPtr<SnapshotMemoryStat> ThreadInMemSnapshot<Profile>::memory_stat(bool include_containers) {
    return pimpl_->compute_memory_stat(include_containers);
}

}}
