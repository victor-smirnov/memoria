
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

#include <memoria/store/memory/common/snapshot_base.hpp>

#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/core/container/allocator.hpp>
#include <memoria/core/container/ctr_impl.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/containers/map/map_factory.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memoria/fiber/shared_mutex.hpp>
#include <memoria/fiber/recursive_shared_mutex.hpp>

#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
namespace store {
namespace memory {

enum class OperationType {OP_FIND, OP_CREATE, OP_UPDATE, OP_DELETE};


template <typename Profile, typename PersistentAllocator>
class FibersSnapshot: public SnapshotBase<Profile, PersistentAllocator, FibersSnapshot<Profile, PersistentAllocator>>
{    
protected:
    using Base = SnapshotBase<Profile, PersistentAllocator, FibersSnapshot<Profile, PersistentAllocator>>;
    using MyType = FibersSnapshot<Profile, PersistentAllocator>;
    
    using typename Base::PersistentAllocatorPtr;
    using typename Base::HistoryNode;
    using typename Base::SnapshotPtr;
    using typename Base::SnapshotApiPtr;
    using typename Base::ApiProfileT;

    using AllocatorMutexT	= typename std::remove_reference<decltype(std::declval<HistoryNode>().allocator_mutex())>::type;
    using MutexT			= typename std::remove_reference<decltype(std::declval<HistoryNode>().snapshot_mutex())>::type;
    using StoreMutexT		= typename std::remove_reference<decltype(std::declval<HistoryNode>().store_mutex())>::type;

    using LockGuardT			= typename std::lock_guard<MutexT>;
    using StoreLockGuardT		= typename std::lock_guard<StoreMutexT>;
    using AllocatorLockGuardT	= typename std::lock_guard<AllocatorMutexT>;
    
    using typename Base::SnapshotID;
    using typename Base::CtrID;

    using Base::history_node_;
    using Base::history_tree_;
    using Base::history_tree_raw_;
    using Base::do_drop;
    using Base::check_tree_structure;

    int32_t cpu_;

    Result<SnapshotApiPtr> upcast(Result<SnapshotPtr>&& ptr) {
        return memoria_static_pointer_cast<IMemorySnapshot<ApiProfileT>>(std::move(ptr));
    }

public:

    using Base::has_open_containers;
    using Base::uuid;
    
    
    FibersSnapshot(MaybeError& maybe_error, HistoryNode* history_node, const PersistentAllocatorPtr& history_tree, OperationType op_type):
        Base(maybe_error, history_node, history_tree),
        cpu_(history_tree->cpu_)
    {
        if (history_tree->event_listener_.get()) {
            if (op_type == OperationType::OP_CREATE) {
                history_tree->event_listener_->shapshot_created(history_node->snapshot_id());
            }
        }
    }

    FibersSnapshot(MaybeError& maybe_error, HistoryNode* history_node, PersistentAllocator* history_tree, OperationType op_type):
        Base(maybe_error, history_node, history_tree),
        cpu_(history_tree->cpu_)
    {
        if (history_tree->event_listener_.get()) {
            if (op_type == OperationType::OP_CREATE) {
                history_tree->event_listener_->shapshot_created(history_node->snapshot_id());
            }
        }
    }

    FibersSnapshot(const FibersSnapshot&) = delete;
    FibersSnapshot(FibersSnapshot&&) = delete;

 
    virtual ~FibersSnapshot() noexcept
    {
        //FIXME This code doesn't decrement properly number of active snapshots
    	// for allocator to store data correctly.

        reactor::engine().run_at_v(cpu_, [&]
        {
            bool drop1 = false;
            bool drop2 = false;

    		if (history_node_->unref() == 0)
    		{
    			if (history_node_->is_active())
    			{
    				drop1 = true;
    			}
    			else if(history_node_->is_dropped())
    			{
    				drop2 = true;
    				check_tree_structure(history_node_->root());
    			}
    		}


            if (drop1)
            {
                do_drop();
                history_tree_raw_->forget_snapshot(history_node_);
            
                //FIXME: do we need to decrese number of active snapshots?
            }

            if (drop2)
            {
                do_drop();
                // FIXME: check if absence of snapshot lock here leads to data races...
            }
        });
    }
    
    

    SnpSharedPtr<SnapshotMetadata<ApiProfileT>> describe() const
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
            std::vector<SnapshotID> children;

            for (const auto& node: history_node_->children())
            {
                children.emplace_back(node->snapshot_id());
            }

            auto parent_id = history_node_->parent() ? history_node_->parent()->snapshot_id() : SnapshotID{};

            return snp_make_shared<SnapshotMetadata<ApiProfileT>>(
                parent_id, history_node_->snapshot_id(), children, history_node_->metadata(), history_node_->status()
            );
        });
    }

    VoidResult commit(bool flush = true) noexcept
    {
        using ResultT = Result<void>;
        return reactor::engine().run_at(cpu_, [&] () noexcept -> ResultT
    	{
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
            if (history_node_->is_active() || history_node_->is_data_locked())
            {
                MEMORIA_TRY_VOID(this->flush_open_containers());
                history_node_->commit();
                history_tree_raw_->unref_active();
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid());
            }

            return Result<void>::of();
        });
    }

    VoidResult drop() noexcept
    {
        return reactor::engine().run_at(cpu_, [&]() noexcept -> VoidResult
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());

            if (history_node_->parent() != nullptr)
            {
                history_node_->mark_to_clear();
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Can't drop root snapshot {}", uuid());
            }

            return VoidResult::of();
        });
    }

    U8String snapshot_metadata() const noexcept
    {
        return reactor::engine().run_at(cpu_, [&]{
            return history_node_->metadata();
        });
    }

    VoidResult set_snapshot_metadata(U8StringRef metadata) noexcept
    {
        return reactor::engine().run_at(cpu_, [&]() noexcept -> VoidResult
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
            if (history_node_->is_active())
            {
                history_node_->set_metadata(metadata);
            }
            else
            {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot is already committed.");
            }

            return VoidResult::of();
        });
    }

    VoidResult lock_data_for_import() noexcept
    {
        return reactor::engine().run_at(cpu_, [&] () noexcept -> VoidResult
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
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
            else if (history_node_->is_data_locked()) {
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid());
            }

            return VoidResult::of();
        });
    }


    Result<SnapshotApiPtr> branch() noexcept
    {
        using ResultT = Result<SnapshotApiPtr>;

        return reactor::engine().run_at(cpu_, [&] () noexcept -> ResultT
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
            if (history_node_->is_committed())
            {
                HistoryNode* history_node = new HistoryNode(history_node_);

                history_tree_raw_->snapshot_map_[history_node->snapshot_id()] = history_node;

                return upcast(snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this(), OperationType::OP_CREATE));
            }
            else if (history_node_->is_data_locked()){
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is locked, branching is not possible.", uuid());
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is still being active. Commit it first.", uuid());
            }
        });
    }

    bool has_parent() const noexcept
    {
    	return reactor::engine().run_at(cpu_, [&] {
            return history_node_->parent() != nullptr;
        });
    }

    Result<SnapshotApiPtr> parent() noexcept
    {
        using ResultT = Result<SnapshotApiPtr>;
        return reactor::engine().run_at(cpu_, [&] () noexcept -> ResultT {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            if (history_node_->parent())
            {
                HistoryNode* history_node = history_node_->parent();
                return upcast(snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this(), OperationType::OP_FIND));
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has no parent.", uuid());
            }
        });
    }

    

    Result<SharedPtr<SnapshotMemoryStat<ApiProfileT>>> memory_stat() noexcept
    {
        using ResultT = Result<SharedPtr<SnapshotMemoryStat<ApiProfileT>>>;
        std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());
        return ResultT::of(this->do_compute_memory_stat());
    }

    Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>> snapshot_ref_creation_allowed() noexcept
    {
        using ResultT = Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>>;
        MEMORIA_TRY_VOID(this->checkIfConainersCreationAllowed());
        return ResultT::of(memoria_static_pointer_cast<AllocatorApiBase<ApiProfileT>>(this->shared_from_this()));
    }


    Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>> snapshot_ref_opening_allowed() noexcept
    {
        using ResultT = Result<SnpSharedPtr<AllocatorApiBase<ApiProfileT>>>;
        MEMORIA_TRY_VOID(this->checkIfConainersOpeneingAllowed());
        return ResultT::of(memoria_static_pointer_cast<AllocatorApiBase<ApiProfileT>>(this->shared_from_this()));
    }
};

}}
}
