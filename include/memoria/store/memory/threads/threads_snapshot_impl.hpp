
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

#include <memoria/core/tools/pool.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/memory.hpp>
#include <memoria/containers/map/map_factory.hpp>
#include <memoria/core/tools/pair.hpp>
#include <memoria/core/tools/type_name.hpp>



#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
namespace store {
namespace memory {


template <typename Profile, typename PersistentAllocator>
class ThreadsSnapshot: public IVertex, public SnapshotBase<Profile, PersistentAllocator, ThreadsSnapshot<Profile, PersistentAllocator>>
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
    
    using Base::history_node_;
    using Base::history_tree_;
    using Base::history_tree_raw_;
    using Base::do_drop;
    using Base::check_tree_structure;

    template <typename>
    friend class ThreadMemoryStoreImpl;

public:
    using Base::has_open_containers;
    using Base::uuid;
    
    
    ThreadsSnapshot(HistoryNode* history_node, const PersistentAllocatorPtr& history_tree):
        Base(history_node, history_tree)
    {}

    ThreadsSnapshot(HistoryNode* history_node, PersistentAllocator* history_tree):
        Base(history_node, history_tree)
    {
    }
 
    virtual ~ThreadsSnapshot()
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

    Result<void> commit() noexcept
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active() || history_node_->is_data_locked())
        {
            auto res = this->flush_open_containers();
            MEMORIA_RETURN_IF_ERROR(res);

            history_node_->commit();
            history_tree_raw_->unref_active();

            if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
                std::cout << "MEMORIA: COMMIT snapshot: " << history_node_->snapshot_id() << std::endl;
            }
        }
        else {
            return Result<void>::make_error("Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid());
        }

        return Result<void>::of();
    }

    Result<void> drop() noexcept
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
            return Result<void>::make_error("Can't drop root snapshot {}", uuid());
        }

        return Result<void>::of();
    }

    U8String snapshot_metadata() const noexcept
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());
        return history_node_->metadata();
    }

    Result<void> set_snapshot_metadata(U8StringRef metadata) noexcept
    {
    	LockGuardT lock_guard(history_node_->snapshot_mutex());

        if (history_node_->is_active())
        {
            history_node_->set_metadata(metadata);
        }
        else
        {
            return Result<void>::make_error("Snapshot has been already committed.");
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
            auto res = has_open_containers();
            MEMORIA_RETURN_IF_ERROR(res);

            if (!res.get())
    		{
    			history_node_->lock_data();
    		}
    		else {
                return VoidResult::make_error("Snapshot {} has open containers", uuid());
    		}
    	}
    	else if (history_node_->is_data_locked()) {
    	}
    	else {
            return VoidResult::make_error("Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid());
    	}

        return VoidResult::of();
    }


    Result<SnapshotApiPtr> branch() noexcept
    {
        using ResultT = Result<SnapshotApiPtr>;

    	std::lock(history_node_->allocator_mutex(), history_node_->snapshot_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->is_committed())
        {
            HistoryNode* history_node = new HistoryNode(history_node_);

            if (history_tree_raw_->is_dump_snapshot_lifecycle()) {
                std::cout << "MEMORIA: BRANCH snapshot: " << history_node->snapshot_id() << std::endl;
            }

            LockGuardT lock_guard3(history_node->snapshot_mutex());

            history_tree_raw_->snapshot_map_[history_node->snapshot_id()] = history_node;

            return ResultT::of(snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this()));
        }
        else if (history_node_->is_data_locked())
        {
            return ResultT::make_error("Snapshot {} is locked, branching is not possible.", uuid());
        }
        else
        {
            return ResultT::make_error("Snapshot {} is still being active. Commit it first.", uuid());
        }
    }

    bool has_parent() const noexcept
    {
    	AllocatorLockGuardT lock_guard(history_node_->allocator_mutex());
        return history_node_->parent() != nullptr;
    }

    Result<SnapshotApiPtr> parent() noexcept
    {
        using ResultT = Result<SnapshotApiPtr>;
        std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());

    	AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex(), std::adopt_lock);
    	LockGuardT lock_guard1(history_node_->snapshot_mutex(), std::adopt_lock);

        if (history_node_->parent())
        {
            HistoryNode* history_node = history_node_->parent();
            return ResultT::of(snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this()));
        }
        else
        {
            return ResultT::make_error("Snapshot {} has no parent.", uuid());
        }
    }


    // Vertex API

    virtual Result<Vertex> allocator_vertex() noexcept {
        return Result<Vertex>::of(as_vertex());
    }

    Vertex as_vertex() noexcept {
        return Vertex(StaticPointerCast<IVertex>(this->shared_from_this()));
    }

    virtual Graph graph()
    {
        return history_tree_->as_graph();
    }

    virtual Any id() const
    {
        return Any(uuid());
    }

    virtual U8String label() const
    {
        return U8String("snapshot");
    }

    virtual void remove()
    {
        MMA_THROW(GraphException()) << WhatCInfo("Can't remove snapshot with Vertex::remove()");
    }

    virtual bool is_removed() const
    {
        return false;
    }

    virtual Collection<VertexProperty> properties()
    {
        return make_fn_vertex_properties(
            as_vertex(),
            "metadata", [&]{return snapshot_metadata();}
        );
    }

    virtual Collection<Edge> edges(Direction direction)
    {
        std::vector<Edge> edges;

        Vertex my_vx = as_vertex();
        Graph my_graph = this->graph();

        if (is_in(direction))
        {
            if (history_node_->parent())
            {
                auto pn_snp_api = history_tree_->find(history_node_->parent()->snapshot_id()).get_or_terminate();
                SnapshotPtr pn_snp = memoria_static_pointer_cast<MyType>(pn_snp_api);

                edges.emplace_back(DefaultEdge::make(my_graph, "child", pn_snp->as_vertex(), my_vx));
            }
        }

        if (is_out(direction))
        {
            for (auto& child: history_node_->children())
            {
                auto ch_snp_api = history_tree_->find(child->snapshot_id()).get_or_terminate();
                SnapshotPtr ch_snp = memoria_static_pointer_cast<MyType>(ch_snp_api);


                edges.emplace_back(DefaultEdge::make(my_graph, "child", my_vx, ch_snp->as_vertex()));
            }

            auto iter = this->root_map_->ctr_begin().get_or_terminate();
            while (!iter->iter_is_end())
            {
                auto ctr_name   = iter->key();
                auto root_id    = iter->value();

                auto vertex_ptr = DynamicPointerCast<IVertex>(
                     const_cast<MyType*>(this)->from_root_id(root_id, ctr_name).get_or_terminate()
                );

                edges.emplace_back(DefaultEdge::make(my_graph, "container", my_vx, vertex_ptr));

                iter->next().get_or_terminate();
            }
        }

        return STLCollection<Edge>::make(std::move(edges));
    }

    Result<SharedPtr<SnapshotMemoryStat>> memory_stat(bool include_containers) noexcept
    {
        using ResultT = Result<SharedPtr<SnapshotMemoryStat>>;
        std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());
        return ResultT::of(this->do_compute_memory_stat(include_containers));
    }


    Result<SnpSharedPtr<ProfileAllocatorType<Profile>>> snapshot_ref_creation_allowed() noexcept
    {
        using ResultT = Result<SnpSharedPtr<ProfileAllocatorType<Profile>>>;
        this->checkIfConainersCreationAllowed();
        return ResultT::of(memoria_static_pointer_cast<ProfileAllocatorType<Profile>>(this->shared_from_this()));
    }


    Result<SnpSharedPtr<ProfileAllocatorType<Profile>>> snapshot_ref_opening_allowed() noexcept
    {
        using ResultT = Result<SnpSharedPtr<ProfileAllocatorType<Profile>>>;
        this->checkIfConainersOpeneingAllowed();
        return ResultT::of(memoria_static_pointer_cast<ProfileAllocatorType<Profile>>(this->shared_from_this()));
    }
};

}}
}
