
// Copyright 2020-2025 Victor Smirnov
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

#include <memoria/store/oltp/oltp_store_snapshot_base.hpp>
#include <memoria/store/swmr/common/allocation_pool.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <memoria/store/oltp/block_id_set.hpp>



namespace memoria {

template <typename Profile> class OLTPStoreBase;
template <typename Profile> class OLTPStoreReadOnlySnapshotBase;

struct InitOLTPStoreTag{};


template <typename Profile>
class OLTPStoreWritableSnapshotBase:
        public OLTPStoreSnapshotBase<Profile>,
        public ISWMRStoreWritableSnapshot<ApiProfile<Profile>>
{
protected:
    using Base = OLTPStoreSnapshotBase<Profile>;

    using typename ISWMRStoreWritableSnapshot<ApiProfile<Profile>>::ROStoreSnapshotPtr;
    using MyType = OLTPStoreWritableSnapshotBase;

    using typename Base::Store;
    using typename Base::CDescrPtr;
    using typename Base::SnapshotDescriptorT;
    using typename Base::SnapshotID;
    using typename Base::BlockID;
    using typename Base::BlockIDValueHolder;
    using typename Base::SharedBlockPtr;
    using typename Base::SharedBlockConstPtr;
    using typename Base::BlockType;

    using typename Base::ApiProfileT;
    using typename Base::StoreT;

    using typename Base::EvcQueueCtr;
    using typename Base::EvcQueueCtrType;
    using typename Base::Superblock;
    using typename Base::Shared;

    using typename Base::DirectoryCtrType;

    using Base::ref_block;
    using Base::unref_block;
    using Base::unref_ctr_root;

    using Base::directory_ctr_;    
    using Base::evc_queue_ctr_;
    using Base::createCtrName;
    using Base::getRootID;
    using Base::getBlock;
    using Base::instance_pool;

    using Base::store_;
    using Base::snapshot_descriptor_;
    using Base::get_superblock;
    using Base::is_transient;
    using Base::is_system_snapshot;

    //using Base::ALLOCATION_MAP_LEVELS;
    //using Base::LAST_ALLOCATION_LEVEL;
    using Base::BASIC_BLOCK_SIZE;
    //using Base::SUPERBLOCK_ALLOCATION_LEVEL;
    using Base::SUPERBLOCK_SIZE;
//    using Base::SUPERBLOCKS_RESERVED;
    //using Base::MAX_BLOCK_SIZE;

    using CtrID = ProfileCtrID<Profile>;
    using CtrReferenceableResult = Result<CtrReferenceable<ApiProfile<Profile>>>;

    using ParentCommit        = SnpSharedPtr<OLTPStoreReadOnlySnapshotBase<Profile>>;

    using BlockCleanupHandler = std::function<void ()>;


    //bool init_store_mode_{};

    bool allocator_map_cloned_{};

    class FlagScope {
        bool& flag_;
    public:
        FlagScope(bool& flag)  : flag_(flag) {
            flag_ = true;
        }
        ~FlagScope() noexcept  {
            flag_ = false;
        }
    };

    template <typename TT>
    class CounterScope {
        TT& cntr_;
    public:
        CounterScope(TT& cntr)  : cntr_(cntr) {
            ++cntr_;
        }
        ~CounterScope() noexcept {
            --cntr_;
        }
    };

    SnapshotID parent_snapshot_id_;


    ArenaBuffer<BlockID> postponed_evictions_;

    CDescrPtr head_snapshot_descriptor_{};
    CDescrPtr consistency_point_snapshot_descriptor_{};

    enum class State {
        ACTIVE, PREPARED, COMMITTED, ROLLED_BACK
    };

    State state_{State::ACTIVE};

    bool do_consistency_point_{false};

    std::unordered_map<BlockID, SharedBlockPtr> my_blocks_;
    std::vector<uint64_t> eviction_queue_buf_;

//    static constexpr io::DevSizeT SIZE_INCREMENT_UNIT = 1ull << (ALLOCATION_MAP_LEVELS - 1); // 1M in 4K blocks
//    static constexpr io::DevSizeT SIZE_INCREMENT_UNIT_BYTES = SIZE_INCREMENT_UNIT * BASIC_BLOCK_SIZE; // 1M
//    static constexpr io::DevSizeT INITIAL_STORE_SIZE = 1; // In units, equivalent to 1MB

//    io::DevSizeT store_size_increment_{16}; // in size increment units

public:
    using Base::check;
    using Base::resolve_block;
    using Base::resolve_block_allocation;
    using Base::CustomLog2;
    using Base::block_provider_;

    OLTPStoreWritableSnapshotBase(
        SharedPtr<Store> store,
        CDescrPtr& snapshot_descriptor
    )
    noexcept :
        Base(store, snapshot_descriptor)
    {
        state_ = State::ACTIVE;
        this->writable_ = true;
    }

    auto sequence_id() const {
        return snapshot_descriptor_->sequence_id();
    }

    virtual bool remove_snapshot(const SnapshotID& snapshot_id) {
        return false;
    }
    virtual bool remove_branch(U8StringView branch_name) {
        return false;
    }

    // FIXME: We probably don't need both.
    virtual SnpSharedPtr<StoreT> my_self_ptr()  = 0;
    virtual SnpSharedPtr<StoreT> self_ptr() {
        return my_self_ptr();
    }

    void cleanup_data() {
        directory_ctr_->cleanup();
    }


    void open_snapshot()
    {
        auto sb = get_superblock();

        auto directory_root_id = sb->directory_root_id();
        if (directory_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(directory_root_id);

            directory_ctr_ = ctr_ref;
            directory_ctr_->internal_detouch_from_store();
        }

        auto evc_queue_root_id = sb->evc_queue_root_id();
        if (evc_queue_root_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<EvcQueueCtrType>(evc_queue_root_id);

            evc_queue_ctr_ = ctr_ref;
            evc_queue_ctr_->internal_detouch_from_store();
        }

        handle_open_snapshot(sb);
    }

    virtual void handle_open_snapshot(io::BlockPtr<Superblock> sb) {}

    virtual io::BlockPtr<Superblock> allocate_superblock(
            const Superblock* parent_sb,
            const SnapshotID& snapshot_id
    ) = 0;

    void init_snapshot(
            CDescrPtr& consistency_point,
            CDescrPtr& head
    )
    {
        //snapshot_descriptor_->set_parent(parent_snapshot_descriptor.get());
        //parent_snapshot_id_ = parent_snapshot_descriptor->snapshot_id();

        // HEAD is always defined here
        head_snapshot_descriptor_ = head;
        consistency_point_snapshot_descriptor_ = consistency_point;

//        auto head_snp = store_->do_open_readonly(head);
//        auto head_allocation_map_ctr = head_snp->find(AllocationMapCtrID);
//        head_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(head_allocation_map_ctr);

//        // HEAD snapshot is not a consistency point
//        if (head != consistency_point)
//        {
//            auto consistency_point_snp = store_->do_open_readonly(consistency_point);
//            auto cp_allocation_map_ctr = consistency_point_snp->find(AllocationMapCtrID);
//            consistency_point_allocation_map_ctr_ = memoria_static_pointer_cast<AllocationMapCtr>(cp_allocation_map_ctr);
//        }

        auto head_sb = get_superblock(head_snapshot_descriptor_->superblock_id());
        auto sb = this->allocate_superblock(
            head_sb.get(),
            ProfileTraits<Profile>::make_random_snapshot_id()
        );

        snapshot_descriptor_->set_superblock(sb.get());

        internal_init_system_ctr<EvcQueueCtrType>(
            evc_queue_ctr_,
            sb->evc_queue_root_id(),
            EvcQueueCtrID
        );

        internal_init_system_ctr<DirectoryCtrType>(
            directory_ctr_,
            sb->directory_root_id(),
            DirectoryCtrID
        );

        handle_init_snapshot(sb);

        eviction_queue_buf_.push_back(encode_txnid(snapshot_descriptor_->sequence_id()));

        trim_eviction_queue();
    }

    virtual void trim_eviction_queue() = 0;
    virtual void handle_init_snapshot(io::BlockPtr<Superblock> sb) {}



    uint64_t encode_txnid(uint64_t txn_id) {
        return txn_id << 4 | 0xF;
    }

    uint64_t dencode_txnid(uint64_t value) {
        return value >> 4;
    }

    bool is_txnid(uint64_t value) const {
        return (value & 0xF) == 0xF;
    }

    virtual void init_store_snapshot() = 0;
    virtual void finish_store_initialization(io::DevSizeT blocks_4K) {}




    virtual void finish_snapshot_opening()
    {

    }


    void flush_eviction_queue_buffer()
    {
        evc_queue_ctr_->append(eviction_queue_buf_);
        eviction_queue_buf_.clear();
    }


    virtual void flush_buffers() = 0;

    uint64_t find_oldest_reader() {
        return 0;
    }


    void prepare(ConsistencyPoint cp)
    {
        if (is_active())
        {
            // FIXME: Check that shapshot is not in the PREPARED state

            if (consistency_point_snapshot_descriptor_) {
                consistency_point_snapshot_descriptor_->inc_snapshots();
            }

            if (cp == ConsistencyPoint::AUTO) {
                do_consistency_point_ =
                        consistency_point_snapshot_descriptor_
                    ->should_make_consistency_point(snapshot_descriptor_);
            }
            else {
                do_consistency_point_ = cp == ConsistencyPoint::YES || cp == ConsistencyPoint::FULL;
            }

            io::BlockPtr<Superblock> sb = get_superblock();

            flush_open_containers();

            flush_buffers();

            flush_allocations(sb);

            store_->do_prepare(Base::snapshot_descriptor_, cp, do_consistency_point_, sb);

            state_ = State::PREPARED;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} has been already closed", snapshot_id()).do_throw();
        }
    }

    virtual void flush_allocations(io::BlockPtr<Superblock> sb) = 0;


    virtual void commit(ConsistencyPoint cp)
    {
        if (is_active()) {
            prepare(cp);
        }

        store_->do_commit(Base::snapshot_descriptor_, do_consistency_point_, this);
        state_ = State::COMMITTED;
    }

    void rollback()
    {
        store_->do_rollback(Base::snapshot_descriptor_);
        state_ = State::ROLLED_BACK;
    }

    virtual SnapshotID snapshot_id()  {
        return Base::snapshot_id();
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const hermes::Datatype& decl, const CtrID& ctr_id)
    {
        checkIfConainersCreationAllowed();
        return this->create_ctr_instance(decl, ctr_id);
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> create(const hermes::Datatype& decl)
    {
        checkIfConainersCreationAllowed();
        auto ctr_id = createCtrName();
        return this->create_ctr_instance(decl, ctr_id);
    }

    static constexpr uint64_t block_size_at(int32_t level)  {
        return (1ull << (level - 1)) * BASIC_BLOCK_SIZE;
    }

    virtual void flush_open_containers() {
        this->instance_pool().for_each_open_ctr(this->self_ptr(), [](auto ctr_id, auto ctr){
            ctr->flush();
        });
    }

    virtual bool drop_ctr(const CtrID& ctr_id)
    {
        check_updates_allowed();

        auto ctr = this->find(ctr_id);
        if (ctr) {
            ctr->drop();
            return true;
        }
        else {
            return false;
        }
    }

    CtrID clone_ctr(const CtrID& ctr_name) {
        return clone_ctr(ctr_name, CtrID{});
    }

    CtrID clone_ctr(const CtrID& ctr_name, const CtrID& new_ctr_name)
    {
        auto root_id = getRootID(ctr_name);
        auto block = getBlock(root_id);

        if (block)
        {
            auto ctr_hash = block->ctr_type_hash();
            auto ctr_intf = ProfileMetadata<Profile>::local()->get_container_operations(ctr_hash);

            return ctr_intf->clone_ctr(ctr_name, new_ctr_name, self_ptr());
        }
        else {
            auto sb = get_superblock();
            MEMORIA_MAKE_GENERIC_ERROR("Container with name {} does not exist in snapshot {} ", ctr_name, sb->snapshot_id()).do_throw();
        }
    }


    virtual void set_transient(bool transient)
    {
        check_updates_allowed();
        //snapshot_descriptor_->set_transient(transient);
    }




    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> find(const CtrID& ctr_id) {
        return Base::find(ctr_id);
    }

    virtual bool is_committed() const  {
        return state_ == State::COMMITTED;
    }

    virtual bool is_active() const  {
        return state_ == State::ACTIVE;
    }

    virtual bool is_marked_to_clear() const  {
        return Base::is_marked_to_clear();
    }


    virtual void dump_open_containers() {
        return Base::dump_open_containers();
    }

    virtual bool has_open_containers() {
        return Base::has_open_containers();
    }

    virtual std::vector<CtrID> container_names() const {
        return Base::container_names();
    }

    virtual void drop() {
        return Base::drop();
    }

    virtual void check(const CheckResultConsumerFn& consumer) {
        return Base::check(consumer);
    }

    virtual Optional<U8String> ctr_type_name_for(const CtrID& name) {
        return Base::ctr_type_name_for(name);
    }

    virtual void walk_containers(ContainerWalker<Profile>* walker, const char* allocator_descr = nullptr) {
        return Base::walk_containers(walker, allocator_descr);
    }

    virtual void setRoot(const CtrID& ctr_id, const BlockID& root)
    {
        auto sb = get_superblock();
        if (MMA_UNLIKELY(ctr_id == DirectoryCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);

                auto prev_id = sb->directory_root_id();
                sb->directory_root_id() = root;
                if (prev_id.is_set())
                {
                    unref_ctr_root(prev_id);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Containers directory removal attempted").do_throw();
            }
        }
        else if (MMA_UNLIKELY(ctr_id == AllocationMapCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);
                allocator_map_cloned_ = true;

                auto prev_id = sb->allocator_root_id();
                sb->allocator_root_id() = root;

                if (prev_id.is_set())
                {
                    unref_ctr_root(prev_id);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("AllocationMap removal attempted").do_throw();
            }
        }
        else if (MMA_UNLIKELY(ctr_id == EvcQueueCtrID))
        {
            if (!root.is_null())
            {
                ref_block(root);

                auto prev_id = sb->evc_queue_root_id();
                sb->evc_queue_root_id() = root;
                if (prev_id.is_set())
                {
                    unref_ctr_root(prev_id);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Eviction Queue removal attempted").do_throw();
            }
        }
        else {
            if (root.is_set())
            {
                ref_block(root);

                auto prev_id = directory_ctr_->replace_and_return(ctr_id, root);         
                if (prev_id)
                {
                    unref_ctr_root(prev_id.value());
                }
            }
            else {
                auto prev_id = directory_ctr_->remove_and_return(ctr_id);
                if (prev_id)
                {
                    unref_ctr_root(prev_id.value());
                }
            }
        }
    }

    void checkIfConainersCreationAllowed() {
    }

    void check_updates_allowed() {
    }



    template <typename CtrName, typename CtrInstanceVar>
    void internal_init_system_ctr(            
            CtrInstanceVar& assign_to,
            const BlockID& root_block_id,
            const CtrID& ctr_id
    )
    {
        if (root_block_id.is_set())
        {
            auto ctr_ref = this->template internal_find_by_root_typed<CtrName>(root_block_id);
            assign_to = ctr_ref;
        }
        else {
            auto ctr_ref = this->template internal_create_by_name_typed<CtrName>(ctr_id);
            assign_to = ctr_ref;
        }
    }



    virtual void unref_block(const BlockID& block_id)
    {
        auto block = this->getBlock(block_id);

        auto ctr_hash   = block->ctr_type_hash();
        auto block_hash = block->block_type_hash();

        auto blk_intf = ProfileMetadata<Profile>::local()
                ->get_block_operations(ctr_hash, block_hash);

        blk_intf->for_each_child(block.block(), [&](const BlockID& child_id){
            unref_block(child_id);
        });

        remove_block(block_id);
    }

    virtual void unref_ctr_root(const BlockID& root_block_id)
    {
        return unref_block(root_block_id);
    }


    virtual SnpSharedPtr<IStoreApiBase<ApiProfileT>> snapshot_ref_creation_allowed() {
        return SnpSharedPtr<IStoreApiBase<ApiProfileT>>{};
    }


    virtual SnpSharedPtr<IStoreApiBase<ApiProfileT>> snapshot_ref_opening_allowed() {
        return Base::snapshot_ref_opening_allowed();
    }


    bool is_transient()  {
        return snapshot_descriptor_->is_transient();
    }

    bool is_system_snapshot()  {
        return snapshot_descriptor_->is_system_snapshot();
    }


    virtual void remove_block(const BlockID& id) = 0;

    io::DevSizeT get_block_addr(const BlockID& value) {
        return value.value().value();
    }

    BlockID blk_id_from_raw(uint64_t value) {
        return UID64::from_raw(value);
    }

    void push_to_eviction_queue(const BlockID& block_id) {
        eviction_queue_buf_.push_back(block_id.value());
    }


    virtual SharedBlockPtr clone_block(const SharedBlockConstPtr& block) = 0;
    virtual SharedBlockPtr allocate_block(size_t level) = 0;


    virtual SharedBlockPtr cloneBlock(const SharedBlockConstPtr& block, const CtrID& ctr_id)
    {
        check_updates_allowed();

        auto new_block = clone_block(block);

        new_block->snapshot_id() = snapshot_id();
        new_block->set_references(0);

        my_blocks_[block->id()] = new_block;

        push_to_eviction_queue(block->id());

        // It's not a good idea to drain free_db_mem_ on a threshold into
        // freedb_ctr_ here because of the btree's self-referentiality.

        return new_block;
    }



    virtual SharedBlockPtr createBlock(int32_t initial_size, const CtrID& ctr_id)
    {
        check_updates_allowed();

        if (initial_size == -1) {
            initial_size = BASIC_BLOCK_SIZE;
        }

        size_t scale_factor = initial_size / BASIC_BLOCK_SIZE;
        size_t level = CustomLog2(scale_factor);

        auto new_block = allocate_block(level);

        BlockType* block = new_block.block();
        block->snapshot_id() = snapshot_id();
        my_blocks_[block->id()] = new_block;

        return new_block;
    }

    virtual CtrSharedPtr<CtrReferenceable<ApiProfileT>> internal_create_by_name(
            const hermes::Datatype& decl, const CtrID& ctr_id
    )
    {
        auto ptr = this->create_ctr_instance(decl, ctr_id);
        ptr->internal_detouch_from_store();
        return ptr;
    }

    void import_new_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        check_updates_allowed();

        MyType* txn = memoria_dynamic_pointer_cast<MyType>(ptr).get();

        auto root_id = getRootID(name);
        if (root_id)
        {
            auto root_id = txn->getRootID(name);

            if (root_id)
            {
                setRoot(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR(
                    "Unexpected empty root ID for container {} in snapshot {}",
                    name, txn->snapshot_id()
                ).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Container with name {} already exists in snapshot {}",
                name, snapshot_id()
            ).do_throw();
        }
    }

    void import_ctr_from(ROStoreSnapshotPtr ptr, const CtrID& name)
    {
        check_updates_allowed();

        MyType* txn = memoria_dynamic_pointer_cast<MyType>(ptr).get();

        auto root_id = getRootID(name);
        if (root_id)
        {
            auto root_id = txn->getRootID(name);
            if (root_id)
            {
                setRoot(name, root_id);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR(
                    "Unexpected empty root ID for container {} in snapshot {}",
                    name, txn->snapshot_id()
                ).do_throw();
            }
        }
        else {
            return import_new_ctr_from(ptr, name);
        }
    }

//    bool update_snapshot_metadata(const SnapshotID& snapshot_id, std::function<void (SWMRSnapshotMetadata<ApiProfileT>&)> fn)
//    {
//        bool exists = true;
//        history_ctr_->with_value(snapshot_id, [&](const auto& value) {
//            using ResultT = std::decay_t<decltype(value)>;
//            if (value) {
//                auto vv = value.get().value_t();
//                fn(vv);
//                return ResultT{vv};
//            }
//            else {
//                exists = false;
//                return ResultT{};
//            }
//        });
//        return exists;
//    }

//    bool set_transient(const SnapshotID& snapshot_id, bool value) {
//        return update_snapshot_metadata(snapshot_id, [](auto& meta){
//            meta.set_transient(true);
//        });
//    }

//    bool set_removed_branch(const SnapshotID& snapshot_id, bool value) {
//        return update_snapshot_metadata(snapshot_id, [](auto& meta){
//            meta.set_removed_branch(true);
//        });
//    }


    bool visit_snapshot(const SnapshotDescriptorT* descr)
    {
        return false;
//        auto ii = branch_removal_counters_.find(descr->snapshot_id());
//        if (ii != branch_removal_counters_.end())
//        {
//            if (ii->second.inc() < descr->children().size()) {
//                return false;
//            }
//            else {
//                return true;
//            }
//        }
//        else {
//            branch_removal_counters_.insert(std::make_pair(descr->snapshot_id(), RWCounter{1}));
//            return descr->children().size() <= 1;
//        }
    }

//    bool remove_branch(U8StringView branch_name)
//    {
//        if (removing_branches_.find(branch_name) != removing_branches_.end()) {
//           return true;
//        }

//        removing_branches_.insert(branch_name);

//        using LockGuard = std::lock_guard<std::recursive_mutex>;

//        LockGuard rlock(store_->history_mutex());
//        auto head = store_->history_tree().get_branch_head(branch_name);
//        if (head)
//        {
//            SnapshotDescriptorT* cd = head.get();

//            while (cd && visit_snapshot(cd))
//            {
//                if (!cd->is_transient()){
//                    remove_snapshot(cd->snapshot_id());
//                }

//                cd = cd->parent();
//            }

//            set_removed_branch(cd->snapshot_id(), true);
//            return true;
//        }
//        else {
//            return false;
//        }
//    }



//    bool remove_snapshot(const SnapshotID& snapshot_id)
//    {
//        auto ii = removing_snapshots_.find(snapshot_id);
//        if (ii == removing_snapshots_.end())
//        {
//            if (set_transient(snapshot_id, true)) {
//                removing_snapshots_.insert(snapshot_id);
//            }
//            else {
//                return false;
//            }
//        }

//        return true;
//    }


};

}
