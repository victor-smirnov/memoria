
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
    using typename Base::Superblock;
    using typename Base::CommitDescriptorT;
    using typename Base::CounterStorageT;

    using Base::BASIC_BLOCK_SIZE;
    using Base::head_ptr_;
    using Base::former_head_ptr_;
    using Base::do_open_readonly;
    using Base::eviction_queue_;
    using Base::block_counters_;
    using Base::persistent_commits_;
    using Base::writer_mutex_;
    using Base::do_create_writable;
    using Base::do_create_writable_for_init;

    Span<uint8_t> buffer_;


public:
    MappedSWMRStoreBase() noexcept : Base() {}

    Superblock* get_superblock(uint64_t file_pos) noexcept {
        return ptr_cast<Superblock>(buffer_.data() + file_pos);
    }

    virtual VoidResult do_open_buffer() noexcept
    {
        Superblock* sb0 = ptr_cast<Superblock>(buffer_.data());
        Superblock* sb1 = ptr_cast<Superblock>(buffer_.data() + BASIC_BLOCK_SIZE);

        if (!sb0->match_magick()) {
            return MEMORIA_MAKE_GENERIC_ERROR("First SWMR store header magick number mismatch");
        }

        if (!sb1->match_magick()) {
            return MEMORIA_MAKE_GENERIC_ERROR("Second SWMR store header magick number mismatch");
        }

        if (sb0->sequence_id() > sb1->sequence_id())
        {
            head_ptr_ = new CommitDescriptorT(get_superblock(sb0->superblock_file_pos()));

            if (sb1->sequence_id() > 0)
            {
                former_head_ptr_ = new CommitDescriptorT(get_superblock(sb1->superblock_file_pos()));
            }
        }
        else {
            head_ptr_ = new CommitDescriptorT(get_superblock(sb1->superblock_file_pos()));

            if (sb0->sequence_id() > 0)
            {
                former_head_ptr_ = new CommitDescriptorT(get_superblock(sb0->superblock_file_pos()));
            }
        }

        if (head_ptr_->superblock()->file_size() != buffer_.size()) {
            return MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store file size mismatch with header: {} {}",
                head_ptr_->superblock()->file_size(), buffer_.size()
            );
        }

        // Read snapshot history and
        // preload all transient snapshots into the
        // eviction queue
        MEMORIA_TRY(ptr, do_open_readonly(head_ptr_));

        uint64_t head_pos = head_ptr_->superblock()->superblock_file_pos();

        uint64_t former_head_pos{};
        if (former_head_ptr_) {
            former_head_pos = former_head_ptr_->superblock()->superblock_file_pos();
        }

        auto rr = ptr->for_each_history_entry([&](const auto& commit_id, int64_t root_block_addr) noexcept -> VoidResult {
            if (root_block_addr < 0)
            {
                uint64_t superblock_pos = -root_block_addr;
                if (superblock_pos != former_head_pos && superblock_pos != head_pos)
                {
                    auto res = wrap_throwing([&](){
                        Superblock* superblock = ptr_cast<Superblock>(buffer_.data() + superblock_pos);
                        CommitDescriptorT* commit_descr = new CommitDescriptorT(superblock);
                        eviction_queue_.push_back(*commit_descr);
                    });
                    MEMORIA_RETURN_IF_ERROR(res);
                }
            }
            else {
                Superblock* superblock = ptr_cast<Superblock>(buffer_.data() + root_block_addr);
                CommitDescriptorT* commit_descr = new CommitDescriptorT(superblock);
                commit_descr->set_persistent(true);
                persistent_commits_[commit_id] = commit_descr;
            }

            return VoidResult::of();
        });
        MEMORIA_RETURN_IF_ERROR(rr);

        if (head_ptr_->superblock()->is_clean()) {
            return read_block_counters();
        }
        else {
            return rebuild_block_counters();
        }

        return VoidResult::of();
    }

    VoidResult read_block_counters() noexcept
    {
        auto ctr_file_pos = head_ptr_->superblock()->block_counters_file_pos();
        auto size = head_ptr_->superblock()->block_counters_size();
        const CounterStorageT* ctr_storage = ptr_cast<const CounterStorageT>(buffer_.data() + ctr_file_pos);

        for (uint64_t c = 0; c < size; c++)
        {
            block_counters_.set(ctr_storage[c].block_id, ctr_storage[c].counter);
        }

        return VoidResult::of();
    }

    VoidResult rebuild_block_counters() noexcept {
        MEMORIA_TRY(commits, this->build_ordered_commits_list());

        for (auto& commit: commits) {
            MEMORIA_TRY_VOID(commit->build_block_refcounters(block_counters_));
        }

        return VoidResult::of();
    }

    VoidResult init_mapped_store() noexcept
    {
        Superblock* sb0 = get_superblock(0);
        Superblock* sb1 = get_superblock(BASIC_BLOCK_SIZE);

        MEMORIA_TRY_VOID(sb0->init(0, buffer_.size(), 0, BASIC_BLOCK_SIZE, 0));
        MEMORIA_TRY_VOID(sb0->build_superblock_description());

        MEMORIA_TRY_VOID(sb1->init(BASIC_BLOCK_SIZE, buffer_.size(), 0, BASIC_BLOCK_SIZE, 0));
        MEMORIA_TRY_VOID(sb1->build_superblock_description());

        //MaybeError maybe_error{};
        CommitDescriptorT* commit_descriptor = new CommitDescriptorT();

        writer_mutex_.lock();

        auto ptr_res = do_create_writable_for_init(commit_descriptor);

//        auto ptr = snp_make_shared<MappedSWMRStoreWritableCommit<Profile>>(
//            maybe_error, this->shared_from_this(), buffer_, commit_descriptor, InitStoreTag{}
//        );

        if (ptr_res.is_ok())
        {
            MEMORIA_TRY_VOID(ptr_res.get()->finish_store_initialization());
            head_ptr_ = commit_descriptor;
            return VoidResult::of();
        }
        else {
            delete commit_descriptor;
            return std::move(ptr_res).transfer_error();
        }
    }


};

}
