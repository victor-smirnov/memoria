
// Copyright 2020-2021 Victor Smirnov
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

#include <memoria/profiles/common/common.hpp>

#include <memoria/store/swmr/common/superblock.hpp>

#include <boost/intrusive/list.hpp>

#include <atomic>

namespace memoria {

template <typename Profile>
class CommitDescriptor: public boost::intrusive::list_base_hook<> {
    using Superblock = SWMRSuperblock<Profile>;
    using BlockID    = ProfileBlockID<Profile>;
    using CommitID   = typename Superblock::CommitID;
    using SequenceID   = typename Superblock::SequenceID;

    std::atomic<int32_t> uses_{};
    Superblock* superblock_;
    bool persistent_{false};
    SequenceID sequence_id_{};
    CommitID commit_id_{};

public:
    CommitDescriptor() noexcept:
        superblock_(nullptr)
    {}

    CommitDescriptor(Superblock* superblock) noexcept:
        superblock_(superblock)
    {
        sequence_id_ = superblock_->sequence_id();
        commit_id_   = superblock_->commit_id();
    }

    Superblock* superblock() noexcept {
        return superblock_;
    }

    void set_superblock(Superblock* superblock) noexcept {
        superblock_ = superblock;
    }

    bool is_persistent() const noexcept {
        return persistent_;
    }

    void set_persistent(bool persistent) noexcept {
        persistent_ = persistent;
    };

    const SequenceID& sequence_id() const noexcept {
        return sequence_id_;
    }

    void ref() noexcept {
        uses_.fetch_add(1);
    }

    void unref() noexcept {
        uses_.fetch_sub(1);
    }

    bool is_in_use() noexcept {
        return uses_.load() > 0;
    }

    const CommitID& commit_id() const noexcept {
        return commit_id_;
    }
};

template <typename Profile>
using CommitDescriptorsList = boost::intrusive::list<CommitDescriptor<Profile>>;

}
