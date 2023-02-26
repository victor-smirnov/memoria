
// Copyright 2019-2021 Victor Smirnov
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

#include <memoria/store/swmr/common/swmr_store_base.hpp>

namespace memoria {


template <typename Profile>
class MappedSWMRStoreBase: public SWMRStoreBase<Profile> {
    using Base = SWMRStoreBase<Profile>;
protected:
    using typename Base::SuperblockT;
    using typename Base::CounterBlockT;
    using typename Base::SnapshotDescriptorT;
    using typename Base::CounterStorageT;
    using typename Base::SnapshotID;

    using Base::BASIC_BLOCK_SIZE;

    using Base::history_tree_;
    using Base::do_open_readonly;
    using Base::block_counters_;
    using Base::writer_mutex_;
    using Base::do_create_writable;
    using Base::do_create_writable_for_init;
    using Base::read_only_;

    Span<uint8_t> buffer_;



public:
    using Base::do_open_store;

    MappedSWMRStoreBase()  : Base() {}

    virtual void store_superblock(SuperblockT* superblock, uint64_t sb_slot) override {
        std::memcpy(buffer_.data() + sb_slot * BASIC_BLOCK_SIZE, superblock, BASIC_BLOCK_SIZE);
    }

    SharedSBPtr<SuperblockT> get_superblock(uint64_t file_pos) override
    {
        SuperblockT* sb = ptr_cast<SuperblockT>(buffer_.data() + file_pos);
        auto shared = new detail::MMapSBPtrNewSharedImpl();
        return SharedSBPtr<SuperblockT>{sb, shared};
    }

    SharedSBPtr<CounterBlockT> get_counter_block(uint64_t file_pos) override {
        CounterBlockT* sb = ptr_cast<CounterBlockT>(buffer_.data() + file_pos);
        auto shared = new detail::MMapSBPtrNewSharedImpl();
        return SharedSBPtr<CounterBlockT>{sb, shared};
    }

    virtual uint64_t buffer_size() override {
        return buffer_.size();
    }
};

}
