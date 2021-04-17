
// Copyright 2016-2021 Victor Smirnov
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


#include <memoria/store/memory_cow/common/snapshot_base_cow.hpp>

#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/core/container/store.hpp>
#include <memoria/core/container/ctr_impl.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/containers/map/map_factory.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/type_name.hpp>



#include <vector>
#include <memory>
#include <mutex>


namespace memoria {
namespace store {
namespace memory_cow {

template <typename Profile, typename PersistentAllocator>
class ThreadsSnapshot: public SnapshotBase<Profile, PersistentAllocator, ThreadsSnapshot<Profile, PersistentAllocator>>
{    
protected:
    using Base = SnapshotBase<Profile, PersistentAllocator, ThreadsSnapshot<Profile, PersistentAllocator>>;
    using MyType = ThreadsSnapshot<Profile, PersistentAllocator>;
    
    using typename Base::PersistentAllocatorPtr;
    using typename Base::HistoryNode;
    using typename Base::SnapshotPtr;
    using typename Base::SnapshotApiPtr;

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
    using typename Base::ApiProfileT;
    
    using Base::history_node_;
    using Base::history_tree_;
    using Base::history_tree_raw_;
    using Base::do_drop;


    template <typename>
    friend class ThreadsMemoryStoreImpl;

    Result<SnapshotApiPtr> upcast(Result<SnapshotPtr>&& ptr) {
        return memoria_static_pointer_cast<IMemorySnapshot<ApiProfileT>>(std::move(ptr));
    }

public:
    using Base::has_open_containers;
    using Base::uuid;
    
    
    ThreadsSnapshot(MaybeError& maybe_error, HistoryNode* history_node, const PersistentAllocatorPtr& history_tree):
        Base(maybe_error, history_node, history_tree)
    {
    }

    ThreadsSnapshot(MaybeError& maybe_error, HistoryNode* history_node, PersistentAllocator* history_tree):
        Base(maybe_error, history_node, history_tree)
    {
    }
 
    virtual ~ThreadsSnapshot() noexcept
    {
    	bool drop1 = false;
    	bool drop2 = false;

    	{
    		LockGuardT snapshot_lock_guard(history_node_->snapshot_mutex());

            if (history_node_->unref() == 0 && history_node_->root_id().isSet())
    		{
    			if (history_node_->is_active())
    			{
    				drop1 = true;
                    history_tree_raw_->unref_active();
    			}
    			else if(history_node_->is_dropped())
    			{
    				drop2 = true;
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
            // FIXME: check if absence of snapshot lock here leads to data races...
    		StoreLockGuardT store_lock_guard(history_node_->store_mutex());
    		do_drop();
    	}
    }

    SnapshotMetadata<ApiProfileT> describe() const
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

        return SnapshotMetadata<ApiProfileT>(parent_id, history_node_->snapshot_id(), children, history_node_->metadata(), history_node_->status());
    }

    VoidResult commit(bool flush = true) noexcept
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active())
        {
            MEMORIA_TRY_VOID(this->flush_open_containers());

            history_node_->commit();
            history_tree_raw_->unref_active();

            if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
                std::cout << "MEMORIA: COMMIT snapshot: " << history_node_->snapshot_id() << std::endl;
            }
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid());
        }

        return Result<void>::of();
    }

    VoidResult drop() noexcept
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

            if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
                std::cout << "MEMORIA: MARK snapshot DROPPED: " << history_node_->snapshot_id() << std::endl;
            }
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't drop root snapshot {}", uuid());
        }

        return VoidResult::of();
    }

    U8String snapshot_metadata() const noexcept
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());
        return history_node_->metadata();
    }

    VoidResult set_snapshot_metadata(U8StringRef metadata) noexcept
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot has been already committed.");
        }

        return Result<void>::of();
    }

    VoidResult lock_data_for_import() noexcept
    {
    	std::lock(history_node_->allocator_mutex(), history_node_->snapshot_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

    	if (history_node_->is_active())
    	{
            MEMORIA_TRY(res, has_open_containers());
            if (!res)
    		{
    			history_node_->lock_data();
    		}
    		else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has open containers", uuid());
    		}
    	}
    	else {
            return MEMORIA_MAKE_GENERIC_ERROR("Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid());
    	}

        return VoidResult::of();
    }


    Result<SnapshotApiPtr> branch() noexcept
    {
        //using ResultT = Result<SnapshotApiPtr>;

    	std::lock(history_node_->allocator_mutex(), history_node_->snapshot_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->is_committed())
        {
            HistoryNode* history_node = new HistoryNode(history_node_);
            history_node->ref_root();

            if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
                std::cout << "MEMORIA: BRANCH snapshot: " << history_node->snapshot_id() << std::endl;
            }

            LockGuardT lock_guard3(history_node->snapshot_mutex());

            history_tree_raw_->snapshot_map_[history_node->snapshot_id()] = history_node;

            return upcast(snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this()));
        }
        else
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is still being active. Commit it first.", uuid());
        }
    }

    bool has_parent() const noexcept
    {
    	AllocatorLockGuardT lock_guard(history_node_->allocator_mutex());
        return history_node_->parent() != nullptr;
    }

    Result<SnapshotApiPtr> parent() noexcept
    {
        //using ResultT = Result<SnapshotApiPtr>;
        std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->parent())
        {
            HistoryNode* history_node = history_node_->parent();
            return upcast(snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this()));
        }
        else
        {
            return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has no parent.", uuid());
        }
    }

    Result<SharedPtr<SnapshotMemoryStat<ApiProfileT>>> memory_stat() noexcept
    {
        using ResultT = Result<SharedPtr<SnapshotMemoryStat<ApiProfileT>>>;
        std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());
        return ResultT::of(this->do_compute_memory_stat());
    }


    Result<SnpSharedPtr<StoreApiBase<ApiProfileT>>> snapshot_ref_creation_allowed() noexcept
    {
        using ResultT = Result<SnpSharedPtr<StoreApiBase<ApiProfileT>>>;
        MEMORIA_TRY_VOID(this->checkIfConainersCreationAllowed());
        return ResultT::of(memoria_static_pointer_cast<StoreApiBase<ApiProfileT>>(this->shared_from_this()));
    }


    Result<SnpSharedPtr<StoreApiBase<ApiProfileT>>> snapshot_ref_opening_allowed() noexcept
    {
        using ResultT = Result<SnpSharedPtr<StoreApiBase<ApiProfileT>>>;
        MEMORIA_TRY_VOID(this->checkIfConainersOpeneingAllowed());
        return ResultT::of(memoria_static_pointer_cast<StoreApiBase<ApiProfileT>>(this->shared_from_this()));
    }
};

}}
}
