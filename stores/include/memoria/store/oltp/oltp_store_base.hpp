
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

#include <memoria/store/oltp/oltp_store_readonly_snapshot_base.hpp>
#include <memoria/store/oltp/oltp_store_writable_snapshot_base.hpp>

#include <memoria/store/oltp/oltp_snapshot_history.hpp>
#include <memoria/store/oltp/oltp_superblock.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/api/io/block_level.hpp>

#include <mutex>
#include <unordered_set>
#include <functional>

namespace memoria {

template <typename Profile>
class OLTPStoreBase: public IOLTPStore<ApiProfile<Profile>> {

protected:
    using Base = IOLTPStore<ApiProfile<Profile>>;

    using typename Base::ReadOnlySnapshotPtr;
    using typename Base::WritableSnapshotPtr;
    using typename Base::SnapshotID;
    using typename Base::SequenceID;

    using ApiProfileT = ApiProfile<Profile>;

    using WritableSnapshotT = OLTPStoreWritableSnapshotBase<Profile>;

    using OLTPReadOnlySnapshotPtr     = SnpSharedPtr<OLTPStoreReadOnlySnapshotBase<Profile>>;
    using OLTPWritableSnapshotPtr     = SnpSharedPtr<WritableSnapshotT>;
    using OLTPWritableSnapshotWeakPtr = SnpWeakPtr<OLTPStoreWritableSnapshotBase<Profile>>;

    using SnapshotDescriptorT = OLTPSnapshotDescriptor<Profile>;
    using CDescrPtr = typename SnapshotDescriptorT::SharedPtrT;

    using BlockID   = ProfileBlockID<Profile>;
    using CtrID     = ProfileCtrID<Profile>;

    using SuperblockT = OLTPSuperblock<Profile>;


    static_assert(sizeof(SuperblockT) <= 4096, "Check superblock size!");

public:
    static constexpr size_t  BASIC_BLOCK_SIZE = 4096;

protected:

    mutable std::recursive_mutex writer_mutex_;
    using LockGuard = std::lock_guard<std::recursive_mutex>;

    OLTPHistory<Profile> history_tree_;

    template <typename> friend class OLTPStoreReadonlySnapshotBase;
    template <typename> friend class OLTPStoreWritableSnapshotBase;
    template <typename> friend class OLTPStoreSnapshotBase;

    bool read_only_{false};

    uint64_t cp_allocation_threshold_{100 * 1024 * 1024 / 4096};
    uint64_t cp_snapshots_threshold_{10000};
    uint64_t cp_timeout_{1000}; // 1 second

    //hermes::HermesCtr store_params_;

    bool active_writer_{false};




public:
    using Base::flush;

    OLTPStoreBase() {}


    uint64_t cp_allocation_threshold() const {
        LockGuard lock(writer_mutex_);
        return cp_allocation_threshold_;
    }

    uint64_t cp_snapshot_threshold() const {
        LockGuard lock(writer_mutex_);
        return cp_snapshots_threshold_;
    }

    uint64_t set_cp_allocation_threshold(uint64_t value) {
        LockGuard lock(writer_mutex_);
        auto tmp = cp_allocation_threshold_;
        cp_allocation_threshold_ = value;
        return tmp;
    }

    uint64_t set_cp_snapshot_threshold(uint64_t value) {
        LockGuard lock(writer_mutex_);
        auto tmp = cp_snapshots_threshold_;
        cp_snapshots_threshold_ = value;
        return tmp;
    }

    int64_t cp_timeout() const {
        LockGuard lock(writer_mutex_);
        return cp_timeout_;
    }

    int64_t set_cp_timeout(int64_t value)
    {
        LockGuard lock(writer_mutex_);
        auto tmp = cp_timeout_;
        cp_timeout_ = value;
        return tmp;
    }

    virtual void close() override = 0;

    virtual void flush_data(bool async = false) = 0;
    virtual void flush_header(bool async = false) = 0;

    virtual void check_if_open() = 0;

    virtual OLTPReadOnlySnapshotPtr do_open_readonly(CDescrPtr snapshot_descr) = 0;
    virtual OLTPWritableSnapshotPtr do_create_writable(
            CDescrPtr consistency_point,
            CDescrPtr head,
            CDescrPtr parent,
            CDescrPtr snapshot_descr
    ) = 0;

    virtual OLTPWritableSnapshotPtr do_open_writable(
            CDescrPtr snapshot_descr,
            bool force = false) = 0;

    virtual OLTPWritableSnapshotPtr do_create_writable_for_init(CDescrPtr snapshot_descr) = 0;
    virtual SharedPtr<OLTPStoreBase<Profile>> self_ptr()  = 0;
    virtual void store_superblock(SuperblockT* superblock, uint64_t sb_slot) = 0;

    virtual io::BlockPtr<SuperblockT> get_superblock(size_t sb_num) = 0;
    virtual io::BlockPtr<SuperblockT> get_superblock(const BlockID& id) = 0;

    //virtual uint64_t buffer_size() = 0;

    virtual void do_flush() = 0;


    virtual ReadOnlySnapshotPtr open() override
    {
        check_if_open();

        LockGuard wlock(writer_mutex_);

//        auto snapshot = [&]{
//            LockGuard rlock(writer_mutex_);
//            return history_tree_.get(snapshot_id);
//        }();

//        if (snapshot) {
//            return open_readonly(snapshot.get());
//        }
//        else {
//            make_generic_error_with_source(MA_SRC, "Can't find snapshot {}", snapshot_id).do_throw();
//        }


        return {};
    }



    virtual WritableSnapshotPtr begin() override {
        return {};
    }




    // FIXME: needed only for flush
    WritableSnapshotPtr create_system_snapshot()
    {
        check_if_open();

        writer_mutex_.lock();

        //check_no_writers();

        try {
            LockGuard lock(writer_mutex_);

            U8String last_branch = history_tree_.head()->branch();
            CDescrPtr snapshot_descriptor;// = create_snapshot_descriptor(last_branch);
            snapshot_descriptor->set_system_snapshot(true);

            OLTPWritableSnapshotPtr ptr = do_create_writable(
                        history_tree_.consistency_point1(),
                        history_tree_.head(),
                        history_tree_.head(),
                        std::move(snapshot_descriptor)
            );

            ptr->finish_snapshot_opening();
            return ptr;
        }
        catch (...) {
            //unlock_writer();
            throw;
        }
    }



    virtual void do_prepare(
            CDescrPtr snapshot_descriptor,
            ConsistencyPoint cp,
            bool do_consistency_point,
            io::BlockPtr<SuperblockT> sb
    )
    {
        if (do_consistency_point)
        {
            sb->inc_consistency_point_sequence_id();
            snapshot_descriptor->refresh_descriptor(sb.get());

            sb->build_superblock_description();

            flush_data();

            auto sb_slot = sb->consistency_point_sequence_id() % 2;
            store_superblock(sb.get(), sb_slot);

            auto sb0 = get_superblock(sb_slot * BASIC_BLOCK_SIZE);

            //sb0->set_metadata_doc(store_params_);
            store_superblock(sb0.get(), sb_slot);

            flush_header();
        }
        else {
            sb->build_superblock_description();
        }
    }

    virtual void do_commit(
        CDescrPtr snapshot_descriptor,
        bool do_consistency_point,
        WritableSnapshotT* snapshot
    )
    {
//        for (const auto& entry: snapshot->counters()) {
//            //block_counters_.apply(entry.first, entry.second.value);
//        }

        {
            LockGuard lock(writer_mutex_);

            // We need to cleanup eviction queue before attaching the new snapshot
            // to the history tree. Because attaching may add a new entry to
            // the queue.
            //cleanup_eviction_queue();

            history_tree_.attach_snapshot(snapshot_descriptor);

//            for (const auto& snapshot_id: snapshot->removing_snapshots())
//            {
//                auto descr = history_tree_.get(snapshot_id);
//                if (descr) {
//                    descr->set_transient(true);
//                }
//            }

//            for (const auto& branch_name: snapshot->removing_branches()) {
//                history_tree_.remove_branch(branch_name);
//            }
        }

        //unlock_writer();
    }

    virtual void do_rollback(CDescrPtr snapshot_descriptor)
    {
        //unlock_writer();
    }




    ReadOnlySnapshotPtr open_readonly(CDescrPtr snapshot_descr) {
        return do_open_readonly(snapshot_descr);
    }


    virtual Optional<SequenceID> check(const CheckResultConsumerFn& consumer) override {
        check_if_open();
        return do_check(consumer);
    }

    void for_all_evicting_snapshots(std::function<void (SnapshotDescriptorT*)> fn)
    {
        LockGuard lock(writer_mutex_);
        for (auto& descr: history_tree_.eviction_queue()) {
            fn(&descr);
        }
    }



protected:



    Optional<SequenceID> do_check(const CheckResultConsumerFn& consumer)
    {
        // FIXME:
        // Writer mutex is needed because we are touching counters here.
        // Need another lightweight method to check the data only without
        // taking writer lock

        LockGuard lock(writer_mutex_);

//        allocations_ = make_lite_allocation_map();

//        auto snapshots = build_ordered_snapshots_list();

//        if (snapshots.size() > 0)
//        {
//            SnapshotCheckState<Profile> check_state;

//            for (auto& snapshot: snapshots)
//            {
//                snapshot->add_superblock(*allocations_.get());
//                snapshot->check(consumer);
//                snapshot->build_block_refcounters(check_state.counters);
//            }

//            check_refcounters(check_state.counters, consumer);
//            do_check_allocations(consumer);

//            allocations_->close();
//            allocations_.reset();
//            return snapshots[snapshots.size() - 1]->sequence_id();
//        }
//        else {
//            allocations_->close();
//            allocations_.reset();
//            return Optional<SequenceID>{};
//        }

        return {};
    }



    static bool is_my_block(const uint8_t* mem_block) {
        const SuperblockT* sb0 = ptr_cast<SuperblockT>(mem_block);
        return sb0->match_magick() && sb0->match_profile_hash();
    }



protected:


    virtual void prepare_to_close()
    {
        if (!this->active_writer_)
        {

        }
    }


    virtual void do_open_store()
    {
        io::BlockPtr<SuperblockT> sb0 = get_superblock(0);
        io::BlockPtr<SuperblockT> sb1 = get_superblock(BASIC_BLOCK_SIZE);

        if (!sb0->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("First OLTP store header magick number mismatch: {}, expected {}",
                                       sb0->magic(), SuperblockT::MAGIC
            ).do_throw();
        }

        if (!sb1->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("Second OLTP store header magick number mismatch: {}, expected {}",
                                       sb1->magic(), SuperblockT::MAGIC
            ).do_throw();
        }

        if (!sb0->match_profile_hash()) {
            MEMORIA_MAKE_GENERIC_ERROR("First OLTP store header profile hash mismatch: {}, expected {}",
                                       sb0->profile_hash(),
                                       SuperblockT::PROFILE_HASH
            ).do_throw();
        }

        if (!sb1->match_profile_hash()) {
            MEMORIA_MAKE_GENERIC_ERROR("Second OLTP store header profile hash mismatch: {}, expected {}",
                                       sb1->profile_hash(),
                                       SuperblockT::PROFILE_HASH
            ).do_throw();
        }

        if (sb0->snapshot_id().is_null() && sb1->snapshot_id().is_null()) {
            // the file was only partially initialized, continue
            // the process with full initialization.
            return init_store();
        }

        CDescrPtr consistency_point1_ptr;
        CDescrPtr consistency_point2_ptr;
        io::BlockPtr<SuperblockT> main_sb;

        if (sb0->consistency_point_sequence_id() > sb1->consistency_point_sequence_id())
        {
            main_sb = sb0;

            consistency_point1_ptr = history_tree_.new_snapshot_descriptor(                        
                get_superblock(sb0->id()).get()
            );

            //store_params_ = sb0->cmetadata_doc().clone();

            if (sb1->snapshot_id().is_set())
            {
                consistency_point2_ptr = history_tree_.new_snapshot_descriptor(
                    get_superblock(sb1->id()).get()
                );
            }
        }
        else {
            main_sb = sb1;

            consistency_point1_ptr = history_tree_.new_snapshot_descriptor(
                get_superblock(sb1->id()).get()
            );

            //store_params_ = sb1->cmetadata_doc().clone();

            if (sb0->snapshot_id().is_set())
            {
                consistency_point2_ptr = history_tree_.new_snapshot_descriptor(
                    get_superblock(sb0->id()).get()
                );
            }
        }

        auto cp1_sb = get_superblock(consistency_point1_ptr->superblock_id());

        // Read snapshot history and
        // preload all transient snapshots into the
        // eviction queue
        auto ptr = do_open_readonly(consistency_point1_ptr);

//        using MetaT = std::pair<SnapshotID, SWMRSnapshotMetadata<ApiProfile<Profile>>>;
//        std::vector<MetaT> metas;

//        ptr->for_each_history_entry([&](const auto& snapshot_id, const auto& snapshot_meta) {
//            metas.push_back(MetaT(snapshot_id, snapshot_meta));
//        });

//        history_tree_.load(metas, consistency_point1_ptr.get(), consistency_point2_ptr.get());

//        if (!read_only_)
//        {
////            allocation_pool_.load(
////                cp1_sb->allocation_pool_data()
////            );

////            if (main_sb->is_clean()) {
////                read_block_counters(main_sb);
////            }
////            else {
////                //rebuild_block_counters();
////            }
//        }
    }








    void init_store()
    {
        auto sb0 = get_superblock(0);
        auto sb1 = get_superblock(1);

        sb0->init(SnapshotID{}, 0, 0);
        sb0->build_superblock_description();

        sb1->init(SnapshotID{}, 1, 1);
        sb1->build_superblock_description();

        auto snapshot_descriptor_ptr = history_tree_.new_snapshot_descriptor();

        writer_mutex_.lock();

        auto ptr = do_create_writable_for_init(snapshot_descriptor_ptr);
        ptr->finish_store_initialization(0);
    }

private:
    template <typename T1, typename T2>
    constexpr static T2 DivUp(T1 v, T2 d) {
        return v / d + (v % d > 0);
    }
};

}
