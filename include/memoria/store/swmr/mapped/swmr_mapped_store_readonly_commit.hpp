
// Copyright 2020 Victor Smirnov
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

#include <memoria/store/swmr/mapped/swmr_mapped_store_common.hpp>

namespace memoria {



template <typename Profile>
class MappedSWMRStoreReadOnlyCommit:
        public MappedSWMRStoreCommitBase<Profile>,
        public EnableSharedFromThis<MappedSWMRStoreReadOnlyCommit<Profile>>
{
    using Base = MappedSWMRStoreCommitBase<Profile>;

    using typename Base::Store;
    using typename Base::CommitDescriptorT;
    using typename Base::CtrID;
    using typename Base::CtrReferenceableResult;
    using typename Base::AllocatorT;

    using typename Base::DirectoryCtrType;
    using typename Base::HistoryCtrType;

    using Base::directory_ctr_;
    using Base::superblock_;
    using Base::internal_find_by_root_typed;

public:
    using Base::find;
    using Base::getBlock;

    MappedSWMRStoreReadOnlyCommit(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate = nullptr
    ) noexcept:
        Base(maybe_error, store, buffer, commit_descriptor, refcounter_delegate)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            auto root_block_id = commit_descriptor->superblock()->directory_root_id();
            if (root_block_id.is_set())
            {
                auto directory_ctr_ref = this->template internal_find_by_root_typed<DirectoryCtrType>(root_block_id);
                MEMORIA_RETURN_IF_ERROR(directory_ctr_ref);
                directory_ctr_ = directory_ctr_ref.get();
            }

            return VoidResult::of();
        });
    }

    VoidResult for_each_root_block(const std::function<VoidResult (int64_t)>& fn) noexcept
    {
        auto history_ctr_ref = this->template internal_find_by_root_typed<HistoryCtrType>(superblock_->history_root_id());
        MEMORIA_RETURN_IF_ERROR(history_ctr_ref);
        auto history_ctr = history_ctr_ref.get();

        MEMORIA_TRY(scanner, history_ctr->scanner_from(history_ctr->iterator()));

        bool has_next;
        do {
            for (auto superblock_ptr: scanner.values())
            {
                MEMORIA_TRY_VOID(fn(superblock_ptr));
            }

            MEMORIA_TRY(has_next_res, scanner.next_leaf());
            has_next = has_next_res;
        }
        while (has_next);

        return VoidResult::of();
    }



protected:
    virtual SnpSharedPtr<AllocatorT> self_ptr() noexcept {
        return this->shared_from_this();
    }
};

}
