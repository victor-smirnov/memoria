
// Copyright 2021-2025 Victor Smirnov
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

#include <memoria/store/swmr/common/mapped_swmr_store_base.hpp>

#include <memoria/store/swmr/mapped/swmr_mapped_store_readonly_snapshot_cowlite.hpp>
#include <memoria/store/swmr/mapped/swmr_mapped_store_writable_snapshot_cowlite.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <boost/filesystem/path.hpp>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <mutex>
#include <unordered_set>
#include <functional>

namespace memoria {

template <typename Profile>
class SWMRLiteRawStore: public MappedSWMRStoreBase<Profile>, public EnableSharedFromThis<SWMRLiteRawStore<Profile>> {

    using Base = MappedSWMRStoreBase<Profile>;

    using MappedReadOnlySnapshotPtr = SnpSharedPtr<MappedSWMRStoreReadOnlySnapshot<Profile>>;
    using MappedWritableSnapshotPtr = SnpSharedPtr<MappedSWMRStoreWritableSnapshot<Profile>>;

protected:

    using typename Base::ReadOnlySnapshotPtr;
    using typename Base::WritableSnapshotPtr;

    using typename Base::SWMRReadOnlySnapshotPtr;
    using typename Base::SWMRWritableSnapshotPtr;


    using typename Base::SnapshotID;
    using typename Base::SequenceID;
    using typename Base::CDescrPtr;
    using typename Base::SuperblockT;
    using typename Base::CounterStorageT;
    using typename Base::BlockID;
    using typename Base::LockGuard;
    using typename Base::ApiProfileT;
    using typename Base::RemovingBlockConsumerFn;

    using Base::history_tree_;
    using Base::block_counters_;
    using Base::get_superblock;
    using Base::buffer_;
    using Base::writer_mutex_;
    using Base::prepare_to_close;

    using Base::HEADER_SIZE;
    using Base::BASIC_BLOCK_SIZE;
    using Base::ALLOCATION_MAP_SIZE_STEP;
    using Base::MB;

    bool closed_{false};

public:
    using Base::init_store;
    using Base::do_open_store;

    SWMRLiteRawStore(Span<uint8_t> buffer)  {
        buffer_ = buffer;
    }

    ~SWMRLiteRawStore() noexcept {
        close();
    }

    void do_flush() override {
    }

    virtual void close() override {
        LockGuard lock(writer_mutex_);

        if (!closed_)
        {
            prepare_to_close();
            closed_ = true;
        }
    }

    static void init_profile_metadata()  {
        MappedSWMRStoreWritableSnapshot<Profile>::init_profile_metadata();
    }

    void flush_data(bool async = false) override {
    }

    void flush_header(bool async = false) override {
    }

    virtual SWMRReadOnlySnapshotPtr do_open_readonly(CDescrPtr snapshot_descr) override
    {        
        if (!snapshot_descr->is_read_only_openable()) {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is transient.", snapshot_descr->snapshot_id()).do_throw();
        }

        MaybeError maybe_error{};
        MappedReadOnlySnapshotPtr ptr = snp_make_shared<MappedSWMRStoreReadOnlySnapshot<Profile>>(
                maybe_error, this->shared_from_this(), buffer_, snapshot_descr
        );

        if (!maybe_error) {
            ptr->post_init();
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.value()).do_throw();
        }
    }

    virtual SWMRWritableSnapshotPtr do_create_writable(
            CDescrPtr consistency_point,
            CDescrPtr head,
            CDescrPtr parent,
            CDescrPtr snapshot_descr
    ) override
    {
        MaybeError maybe_error{};
        auto ptr = snp_make_shared<MappedSWMRStoreWritableSnapshot<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, snapshot_descr
        );

        if (!maybe_error) {
            ptr->init_snapshot(consistency_point, head, parent);
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.value()).do_throw();
        }
    }

    virtual SWMRWritableSnapshotPtr do_open_writable(CDescrPtr snapshot_descr, RemovingBlockConsumerFn fn, bool force) override {
        MaybeError maybe_error{};
        MappedWritableSnapshotPtr ptr{};

        if ((!force) && snapshot_descr->is_linked()) {
            MEMORIA_MAKE_GENERIC_ERROR("Snapshot {} is already being removed", snapshot_descr->snapshot_id()).do_throw();
        }

        ptr = snp_make_shared<MappedSWMRStoreWritableSnapshot<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, snapshot_descr, fn
        );


        if (!maybe_error) {
            ptr->open_snapshot();
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.value()).do_throw();
        }
    }

    virtual SWMRWritableSnapshotPtr do_create_writable_for_init(CDescrPtr snapshot_descr) override
    {
        MaybeError maybe_error{};

        auto ptr = snp_make_shared<MappedSWMRStoreWritableSnapshot<Profile>>(
            maybe_error, this->shared_from_this(), buffer_, snapshot_descr
        );

        if (!maybe_error)
        {
            ptr->init_store_snapshot();
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.value()).do_throw();
        }
    }

    virtual void check_if_open() override {
    }

    virtual SharedPtr<SWMRStoreBase<Profile>> self_ptr()  override {
        return this->shared_from_this();
    }

    static bool is_my_block(Span<uint8_t> block)  {
        if (block.size() > BASIC_BLOCK_SIZE * 2) {
            return Base::is_my_block(block.data());
        }
        return false;
    }

    virtual U8String describe() const override {
        return format_u8("SWMRLiteRawStore<{}>", TypeNameFactory<Profile>::name());
    }
};

}
