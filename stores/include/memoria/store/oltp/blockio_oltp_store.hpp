
// Copyright 2019-2023 Victor Smirnov
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

#include <memoria/store/oltp/blockio_oltp_store_base.hpp>

#include <memoria/store/oltp/blockio_oltp_store_readonly_snapshot.hpp>
#include <memoria/store/oltp/blockio_oltp_store_writable_snapshot.hpp>
#include <memoria/store/swmr/common/swmr_store_base.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <mutex>
#include <unordered_set>
#include <functional>

namespace memoria {

template <typename Profile>
class BlockIOOLTPStore: public BlockIOOLTPStoreBase<Profile>, public EnableSharedFromThis<BlockIOOLTPStore<Profile>> {

    using Base = BlockIOOLTPStoreBase<Profile>;

    using BasicReadOnlySnapshotPtr = SnpSharedPtr<BlockIOOLTPStoreReadOnlySnapshot<Profile>>;
    using BasicWritableSnapshotPtr = SnpSharedPtr<BlockIOOLTPStoreWritableSnapshot<Profile>>;

protected:
    using typename Base::ReadOnlySnapshotPtr;
    using typename Base::WritableSnapshotPtr;

    using typename Base::OLTPReadOnlySnapshotPtr;
    using typename Base::OLTPWritableSnapshotPtr;

    using typename Base::SnapshotID;
    using typename Base::SequenceID;
    using typename Base::CDescrPtr;
    using typename Base::BlockID;
    using typename Base::BlockIOPtr;

    using Base::get_superblock;
    using Base::prepare_to_close;
    using Base::writer_mutex_;
    using Base::history_tree_;
    using Base::read_only_;
    using Base::block_io_;

    using Base::HEADER_SIZE;
    using Base::BASIC_BLOCK_SIZE;
    using Base::ALLOCATION_MAP_SIZE_STEP;
    using Base::MB;

    using ApiProfileT   = ApiProfile<Profile>;
    using LockGuard     = std::lock_guard<std::recursive_mutex>;
    using Superblock    = SWMRSuperblock<Profile>;

public:
    using Base::init_store;

    BlockIOOLTPStore(BlockIOPtr blockio):
        Base(blockio)
    {

    }

    ~BlockIOOLTPStore() noexcept {
       close();
    }

//    virtual void do_flush() override
//    {
////        flush_data();
////        flush_header();
//    }


    virtual void close() override
    {
        LockGuard lock(writer_mutex_);
        block_io_->close();
    }

    static void init_profile_metadata()  {
        BlockIOOLTPStoreWritableSnapshot<Profile>::init_profile_metadata();
    }

//    void flush_data(bool async = false) override
//    {
//        //check_if_open();
//    }

//    void flush_header(bool async = false) override
//    {
//        //check_if_open();
//    }







private:

    virtual OLTPReadOnlySnapshotPtr do_open_readonly(CDescrPtr snapshot_descr) override
    {
//        if (!snapshot_descr->is_read_only_openable()) {
//            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is transient.", snapshot_descr->snapshot_id()).do_throw();
//        }

        auto ptr = snp_make_shared<BlockIOOLTPStoreReadOnlySnapshot<Profile>>
        (
            this->shared_from_this(), block_io_, snapshot_descr
        );

        ptr->post_init();
        return std::move(ptr);
    }

    virtual OLTPWritableSnapshotPtr do_open_writable(CDescrPtr snapshot_descr, bool force) override {

//        OLTPWritableSnapshotPtr ptr{};

//        if ((!force) && snapshot_descr->is_linked()) {
//            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is already being removed", snapshot_descr->snapshot_id()).do_throw();
//        }

        BasicWritableSnapshotPtr ptr = snp_make_shared<BlockIOOLTPStoreWritableSnapshot<Profile>>(
            this->shared_from_this(), block_io_, snapshot_descr
        );

        ptr->open_snapshot();
        return std::move(ptr);
    }


    virtual OLTPWritableSnapshotPtr do_create_writable(
            CDescrPtr consistency_point,
            CDescrPtr head,
            CDescrPtr snapshot_descr
    ) //override
    {
        BasicWritableSnapshotPtr ptr = snp_make_shared<BlockIOOLTPStoreWritableSnapshot<Profile>>(
            this->shared_from_this(), block_io_, snapshot_descr
        );

        ptr->init_snapshot(consistency_point, head);
        return ptr;
    }

    virtual OLTPWritableSnapshotPtr do_create_writable_for_init(CDescrPtr snapshot_descr) override
    {
        BasicWritableSnapshotPtr ptr = snp_make_shared<BlockIOOLTPStoreWritableSnapshot<Profile>>(
            this->shared_from_this(), block_io_, snapshot_descr
        );

        ptr->init_store_snapshot();
        return std::move(ptr);
    }

    virtual SharedPtr<OLTPStoreBase<Profile>> self_ptr() override {
        return this->shared_from_this();
    }
};

}
