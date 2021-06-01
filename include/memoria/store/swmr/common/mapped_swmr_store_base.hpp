
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
    using typename Base::CommitID;

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
    MappedSWMRStoreBase() noexcept : Base() {}

    virtual void store_superblock(Superblock* superblock, uint64_t sb_slot) override {
        std::memcpy(buffer_.data() + sb_slot * BASIC_BLOCK_SIZE, superblock, BASIC_BLOCK_SIZE);
    }

    Superblock* get_superblock(uint64_t file_pos) noexcept override {
        return ptr_cast<Superblock>(buffer_.data() + file_pos);
    }

    virtual void do_open_buffer()
    {
        Superblock* sb0 = ptr_cast<Superblock>(buffer_.data());
        Superblock* sb1 = ptr_cast<Superblock>(buffer_.data() + BASIC_BLOCK_SIZE);

        if (!sb0->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("First SWMR store header magick number mismatch: {}, expected {};  {}, expected {}",
                                       sb0->magick1(), Superblock::MAGICK1,
                                       sb0->magick2(), Superblock::MAGICK2
            ).do_throw();
        }

        if (!sb1->match_magick()) {
            MEMORIA_MAKE_GENERIC_ERROR("Second SWMR store header magick number mismatch: {}, expected {};  {}, expected {}",
                                       sb1->magick1(), Superblock::MAGICK1,
                                       sb1->magick2(), Superblock::MAGICK2
            ).do_throw();
        }

        if (!sb0->match_profile_hash()) {
            MEMORIA_MAKE_GENERIC_ERROR("First SWMR store header profile hash mismatch: {}, expected {}",
                                       sb0->profile_hash(),
                                       Superblock::PROFILE_HASH
            ).do_throw();
        }

        if (!sb1->match_profile_hash()) {
            MEMORIA_MAKE_GENERIC_ERROR("Second SWMR store header profile hash mismatch: {}, expected {}",
                                       sb1->profile_hash(),
                                       Superblock::PROFILE_HASH
            ).do_throw();
        }

        if (sb0->commit_id().is_null() && sb1->commit_id().is_null()) {
            // the file was only partially initialized, continue
            // the process.
            return init_mapped_store();
        }

        std::unique_ptr<CommitDescriptorT> head_ptr;
        std::unique_ptr<CommitDescriptorT> former_head_ptr;

        if (sb0->sequence_id() > sb1->sequence_id())
        {
            head_ptr = std::make_unique<CommitDescriptorT>(get_superblock(sb0->superblock_file_pos()));

            if (sb1->commit_id().is_set())
            {
                former_head_ptr = std::make_unique<CommitDescriptorT>(get_superblock(sb1->superblock_file_pos()));
            }
        }
        else {
            head_ptr = std::make_unique<CommitDescriptorT>(get_superblock(sb1->superblock_file_pos()));

            if (sb0->commit_id().is_set())
            {
                former_head_ptr = std::make_unique<CommitDescriptorT>(get_superblock(sb0->superblock_file_pos()));
            }
        }

        if (head_ptr->superblock()->file_size() != buffer_.size()) {
            MEMORIA_MAKE_GENERIC_ERROR(
                "SWMR Store file size mismatch with header: {} {}",
                head_ptr->superblock()->file_size(), buffer_.size()
            ).do_throw();
        }


        // Read snapshot history and
        // preload all transient snapshots into the
        // eviction queue
        auto ptr = do_open_readonly(head_ptr.get());

        using MetaT = std::pair<CommitID, CommitMetadata<ApiProfile<Profile>>>;
        std::vector<MetaT> metas;

        ptr->for_each_history_entry([&](const auto& commit_id, const auto& commit_meta) {
            metas.push_back(MetaT(commit_id, commit_meta));
        });

        history_tree_.load(metas, head_ptr->commit_id(), former_head_ptr ? former_head_ptr->commit_id() : CommitID{});

        if (!read_only_)
        {
            if (head_ptr->superblock()->is_clean()) {
                read_block_counters();
            }
            else {
                rebuild_block_counters();
            }
        }
    }

    void read_block_counters()
    {
        CommitDescriptorT* head_ptr = history_tree_.last_commit();

        auto ctr_file_pos = head_ptr->superblock()->global_block_counters_file_pos();
        auto size = head_ptr->superblock()->global_block_counters_size();
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

        sb0->init(0, buffer_.size(), CommitID{}, BASIC_BLOCK_SIZE, 0);
        sb0->build_superblock_description();

        sb1->init(BASIC_BLOCK_SIZE, buffer_.size(), CommitID{}, BASIC_BLOCK_SIZE, 1);
        sb1->build_superblock_description();


        auto commit_descriptor_ptr = std::make_unique<CommitDescriptorT>();

        writer_mutex_.lock();

        auto head_ptr = commit_descriptor_ptr.get();

        auto ptr = do_create_writable_for_init(commit_descriptor_ptr.release());

        ptr->finish_store_initialization();
        history_tree_.init_store(head_ptr);
    }


};

}
