
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

#include <memoria/v1/store/memory/common/snapshot_base.hpp>

#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/ctr_impl.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/pool.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/memory.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/core/tools/pair.hpp>
#include <memoria/v1/core/tools/type_name.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/fiber/shared_mutex.hpp>
#include <memoria/v1/fiber/recursive_shared_mutex.hpp>

#include <memoria/v1/core/graph/graph.hpp>
#include <memoria/v1/core/graph/graph_default_vertex_property.hpp>

#include <vector>
#include <memory>
#include <mutex>



namespace memoria {
namespace v1 {
namespace store {
namespace memory {

enum class OperationType {OP_FIND, OP_CREATE, OP_UPDATE, OP_DELETE};


template <typename Profile, typename PersistentAllocator>
class FibersSnapshot: public IVertex, public SnapshotBase<Profile, PersistentAllocator, FibersSnapshot<Profile, PersistentAllocator>>
{    
protected:
    using Base = SnapshotBase<Profile, PersistentAllocator, FibersSnapshot<Profile, PersistentAllocator>>;
    using MyType = FibersSnapshot<Profile, PersistentAllocator>;
    
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
    
    using typename Base::SnapshotID;
    using typename Base::CtrID;

    using Base::history_node_;
    using Base::history_tree_;
    using Base::history_tree_raw_;
    using Base::do_drop;
    using Base::check_tree_structure;

    int32_t cpu_;

public:

    using Base::has_open_containers;
    using Base::uuid;
    
    
    FibersSnapshot(HistoryNode* history_node, const PersistentAllocatorPtr& history_tree, OperationType op_type):
        Base(history_node, history_tree),
        cpu_(history_tree->cpu_)
    {
        if (history_tree->event_listener_.get()) {
            if (op_type == OperationType::OP_CREATE) {
                history_tree->event_listener_->shapshot_created(history_node->snapshot_id());
            }
        }
    }

    FibersSnapshot(HistoryNode* history_node, PersistentAllocator* history_tree, OperationType op_type):
        Base(history_node, history_tree),
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

        reactor::engine().run_at(cpu_, [&]
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
    
    

    SnpSharedPtr<SnapshotMetadata<SnapshotID>> describe() const
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

            return snp_make_shared<SnapshotMetadata<SnapshotID>>(
                parent_id, history_node_->snapshot_id(), children, history_node_->metadata(), history_node_->status()
            );
        });
    }

    void commit()
    {
    	return reactor::engine().run_at(cpu_, [&]
    	{
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
            if (history_node_->is_active() || history_node_->is_data_locked())
            {
                this->flush_open_containers();

                history_node_->commit();
                history_tree_raw_->unref_active();
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid state: {} for snapshot {}", (int32_t)history_node_->status(), uuid()));
            }
        });
    }

    void drop()
    {
        return reactor::engine().run_at(cpu_, [&]
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());

            if (history_node_->parent() != nullptr)
            {
                history_node_->mark_to_clear();
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Can't drop root snapshot {}", uuid()));
            }
        });
    }

    U16String snapshot_metadata() const
    {
    	return reactor::engine().run_at(cpu_, [&]{
            return history_node_->metadata();
        });
    }

    void set_snapshot_metadata(U16StringRef metadata)
    {
    	return reactor::engine().run_at(cpu_, [&]
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
            if (history_node_->is_active())
            {
                history_node_->set_metadata(metadata);
            }
            else
            {
                MMA1_THROW(Exception()) << WhatCInfo("Snapshot is already committed.");
            }
        });
    }

    void lock_data_for_import()
    {
        return reactor::engine().run_at(cpu_, [&] 
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
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
        });
    }


    SnapshotApiPtr branch()
    {
        return reactor::engine().run_at(cpu_, [&] 
        {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            
            if (history_node_->is_committed())
            {
                HistoryNode* history_node = new HistoryNode(history_node_);

                history_tree_raw_->snapshot_map_[history_node->snapshot_id()] = history_node;

                return snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this(), OperationType::OP_CREATE);
            }
            else if (history_node_->is_data_locked())
            {
                MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot {} is locked, branching is not possible.", uuid());
            }
            else
            {
                MMA1_THROW(Exception()) << fmt::format_ex(u"Snapshot {} is still being active. Commit it first.", uuid());
            }
        });
    }

    bool has_parent() const
    {
    	return reactor::engine().run_at(cpu_, [&] {
            return history_node_->parent() != nullptr;
        });
    }

    SnapshotApiPtr parent()
    {
        return reactor::engine().run_at(cpu_, [&] {
            AllocatorLockGuardT lock_guard2(history_node_->allocator_mutex());
            if (history_node_->parent())
            {
                HistoryNode* history_node = history_node_->parent();
                return snp_make_shared_init<MyType>(history_node, history_tree_->shared_from_this(), OperationType::OP_FIND);
            }
            else
            {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Snapshot {} has no parent.", uuid()));
            }
        });
    }

    
    // Vertex API

    virtual Vertex allocator_vertex() {
        return as_vertex();
    }

    Vertex as_vertex() {
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

    virtual U16String label() const
    {
        return U16String(u"snapshot");
    }

    virtual void remove()
    {
        MMA1_THROW(GraphException()) << WhatCInfo("Can't remove snapshot with Vertex::remove()");
    }

    virtual bool is_removed() const
    {
        return false;
    }

    virtual Collection<VertexProperty> properties()
    {
        return make_fn_vertex_properties(
            as_vertex(),
            u"metadata", [&]{return snapshot_metadata();}
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
                auto pn_snp_api = history_tree_->find(history_node_->parent()->snapshot_id());
                SnapshotPtr pn_snp = memoria_static_pointer_cast<MyType>(pn_snp_api);

                edges.emplace_back(DefaultEdge::make(my_graph, u"child", pn_snp->as_vertex(), my_vx));
            }
        }

        if (is_out(direction))
        {
            for (auto& child: history_node_->children())
            {
                auto ch_snp_api = history_tree_->find(child->snapshot_id());
                SnapshotPtr ch_snp = memoria_static_pointer_cast<MyType>(ch_snp_api);


                edges.emplace_back(DefaultEdge::make(my_graph, u"child", my_vx, ch_snp->as_vertex()));
            }

            auto iter = this->root_map_->ctr_begin();
            while (!iter->iter_is_end())
            {
                auto ctr_name   = iter->key();
                auto root_id    = iter->value();

                auto vertex_ptr = DynamicPointerCast<IVertex>(
                     const_cast<MyType*>(this)->from_root_id(root_id, ctr_name)
                );

                edges.emplace_back(DefaultEdge::make(my_graph, u"container", my_vx, vertex_ptr));

                iter->next();
            }
        }

        return STLCollection<Edge>::make(std::move(edges));
    }

    SharedPtr<SnapshotMemoryStat> memory_stat(bool include_containers)
    {
        std::lock(history_node_->snapshot_mutex(), history_node_->allocator_mutex());
        return this->do_compute_memory_stat(include_containers);
    }

    SnpSharedPtr<ProfileAllocatorType<Profile>> snapshot_ref_creation_allowed()
    {
        this->checkIfConainersCreationAllowed();
        return memoria_static_pointer_cast<ProfileAllocatorType<Profile>>(this->shared_from_this());
    }


    SnpSharedPtr<ProfileAllocatorType<Profile>> snapshot_ref_opening_allowed()
    {
        this->checkIfConainersOpeneingAllowed();
        return memoria_static_pointer_cast<ProfileAllocatorType<Profile>>(this->shared_from_this());
    }
};

}}
}}
