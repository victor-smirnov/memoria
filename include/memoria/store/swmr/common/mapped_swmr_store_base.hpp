
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

    virtual void do_open_buffer()
    {
        Superblock* sb0 = ptr_cast<Superblock>(buffer_.data());
        Superblock* sb1 = ptr_cast<Superblock>(buffer_.data() + BASIC_BLOCK_SIZE);

        if (!sb0->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("First SWMR store header magick number mismatch").do_throw();
        }

        if (!sb1->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("Second SWMR store header magick number mismatch").do_throw();
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
            MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store file size mismatch with header: {} {}",
                head_ptr_->superblock()->file_size(), buffer_.size()
            ).do_throw();
        }

        // Read snapshot history and
        // preload all transient snapshots into the
        // eviction queue
        auto ptr = do_open_readonly(head_ptr_);

        uint64_t head_pos = head_ptr_->superblock()->superblock_file_pos();

        uint64_t former_head_pos{};
        if (former_head_ptr_) {
            former_head_pos = former_head_ptr_->superblock()->superblock_file_pos();
        }

        ptr->for_each_history_entry([&](const auto& commit_id, int64_t root_block_addr) {
            if (root_block_addr < 0)
            {
                uint64_t superblock_pos = -root_block_addr;
                if (superblock_pos != former_head_pos && superblock_pos != head_pos)
                {
                    Superblock* superblock = ptr_cast<Superblock>(buffer_.data() + superblock_pos);
                    CommitDescriptorT* commit_descr = new CommitDescriptorT(superblock);
                    eviction_queue_.push_back(*commit_descr);
                }
            }
            else {
                Superblock* superblock = ptr_cast<Superblock>(buffer_.data() + root_block_addr);
                CommitDescriptorT* commit_descr = new CommitDescriptorT(superblock);
                commit_descr->set_persistent(true);
                persistent_commits_[commit_id] = commit_descr;
            }
        });

        if (head_ptr_->superblock()->is_clean()) {
            return read_block_counters();
        }
        else {
            return rebuild_block_counters();
        }
    }

    void read_block_counters()
    {
        auto ctr_file_pos = head_ptr_->superblock()->block_counters_file_pos();
        auto size = head_ptr_->superblock()->block_counters_size();
        const CounterStorageT* ctr_storage = ptr_cast<const CounterStorageT>(buffer_.data() + ctr_file_pos);

        for (uint64_t c = 0; c < size; c++)
        {
            block_counters_.set(ctr_storage[c].block_id, ctr_storage[c].counter);
        }
    }

    void rebuild_block_counters() noexcept {
        auto commits = this->build_ordered_commits_list();

        for (auto& commit: commits) {
            commit->build_block_refcounters(block_counters_);
        }
    }

    void init_mapped_store()
    {
        Superblock* sb0 = get_superblock(0);
        Superblock* sb1 = get_superblock(BASIC_BLOCK_SIZE);

        sb0->init(0, buffer_.size(), 0, BASIC_BLOCK_SIZE, 0);
        sb0->build_superblock_description();

        sb1->init(BASIC_BLOCK_SIZE, buffer_.size(), 0, BASIC_BLOCK_SIZE, 0);
        sb1->build_superblock_description();


        auto commit_descriptor_ptr = std::make_unique<CommitDescriptorT>();

        writer_mutex_.lock();

        auto head_ptr = commit_descriptor_ptr.get();

        auto ptr = do_create_writable_for_init(commit_descriptor_ptr.release());

        ptr->finish_store_initialization();
        head_ptr_ = head_ptr;
    }


};

}
