
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
#include <memoria/core/tools/result.hpp>
#include <memoria/core/tools/optional.hpp>

#include <functional>
#include <absl/container/btree_map.h>

namespace memoria {

template <typename Profile>
using BlockRefCounterFn = std::function<BoolResult(const ProfileBlockID<Profile>&)>;


template <typename Profile>
struct SWMRBlockCounters {

    using BlockID = ProfileBlockID<Profile>;

    struct Counter {
        uint64_t value;
        void inc() noexcept {
            ++value;
        }

        bool dec() noexcept {
            return --value == 0;
        }
    };

private:
    absl::btree_map<BlockID, Counter> map_;
public:

    void clear() noexcept {
        map_.clear();
    }

    auto begin() const {
        return map_.begin();
    }

    auto end() const {
        return map_.end();
    }

    size_t size() const noexcept {
        return map_.size();
    }

    void set(const BlockID& block_id, uint64_t counter) noexcept {
        map_[block_id] = Counter{counter};
    }

    bool inc(const BlockID& block_id) noexcept {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            ii->second.inc();
            return false;
        }
        else {
            map_.insert(std::make_pair(block_id, Counter{1}));
            return true;
        }
    }

    BoolResult dec(const BlockID& block_id) noexcept {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            bool res = ii->second.dec();
            if (res) {
                map_.erase(ii);
            }
            return BoolResult::of(res);
        }
        else {
            return make_generic_error_with_source(MA_SRC, "SWMRStore block counter is not found for block {}", block_id);
        }
    }

    VoidResult for_each(std::function<VoidResult(const BlockID&, uint64_t)> fn) const noexcept
    {
        for (auto ii = map_.begin(); ii != map_.end(); ++ii) {
            MEMORIA_TRY_VOID(fn(ii->first, ii->second.value));
        }
        return VoidResult::of();
    }

    Optional<uint64_t> get(const BlockID& block_id) const noexcept {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            return ii->second.value;
        }
        return Optional<uint64_t>{};
    }
};

}
