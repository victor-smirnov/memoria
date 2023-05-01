
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/arena_buffer.hpp>

#include <memoria/profiles/common/common.hpp>
#include <memoria/store/oltp/oltp_store_snapshot_descriptor.hpp>
#include <memoria/store/swmr/common/swmr_store_datatypes.hpp>
#include <memoria/store/oltp/oltp_superblock.hpp>

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <stack>

#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>

namespace memoria {

template <typename Profile>
class OLTPStoreBase;

template <typename Profile>
class OLTPHistory {

    using SnapshotDescriptorT = OLTPSnapshotDescriptor<Profile>;
    using CDescrPtr         = typename SnapshotDescriptorT::SharedPtrT;

    using SnapshotID        = ProfileSnapshotID<Profile>;
    using SequeceID         = uint64_t;

    std::atomic<SnapshotDescriptorT*> head_ptr_;

    CDescrPtr tail_;

    CDescrPtr head_;

public:
    OLTPHistory()  :
        tail_(),
        head_(),
        head_ptr_()
    {}

    template <typename... Args>
    CDescrPtr new_snapshot_descriptor(Args&&... args) {
        return SnapshotDescriptorT::make(std::forward<Args>(args)...);
    }

    SequeceID trim_history()
    {
        if (MMA_LIKELY((bool)tail_))
        {
            auto head_ptr = head_ptr_.load();

            while (tail_->uses() <= 1 && tail_.get() != head_ptr && tail_ != head_)
            {
                tail_ = tail_->next();
                tail_->remove_prev();
            }

            return tail_->sequence_id();
        }
        else {
            return SequeceID{};
        }
    }

    const CDescrPtr& head() const  {
        return head_;
    }

    SnapshotDescriptorT* head_ptr() const  {
        return head_ptr_.load();
    }

    void attach_snapshot(CDescrPtr descr)
    {
        if (MMA_LIKELY((bool)head_)) {
            descr->set_prev(head_);
            head_->set_next(descr);
            head_ = descr;
        }
        else {
            tail_ = head_ = descr;
        }
    }

    void publish_head() {
        head_ptr_ = head_.get();
    }

    void load(CDescrPtr descr)
    {
        if (MMA_UNLIKELY((bool)head_)) {

        }
    }
};

}
