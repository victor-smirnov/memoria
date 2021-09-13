
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
#include <absl/container/btree_set.h>

namespace memoria {

template <typename Profile>
using BlockRefCounterFn = std::function<BoolResult(const ProfileBlockID<Profile>&)>;


template <typename Profile>
struct CounterStorage {
    using BlockID = ProfileBlockID<Profile>;

    BlockID block_id;
    int64_t counter;
};


template <typename Profile>
struct SWMRBlockCounters {

    using BlockID = ProfileBlockID<Profile>;

    struct Counter {
        int64_t value;
        void inc() noexcept {
            ++value;
        }

        bool dec() noexcept {
            return --value == 0;
        }
    };

private:
    absl::btree_set<BlockID> ctr_roots_;
    absl::btree_map<BlockID, Counter> map_;

public:
    SWMRBlockCounters() {}

    int ccnt_{};
    int dcnt_{};

    void apply(const BlockID& block_id, int64_t value)
    {
        if (value != 0)
        {
            auto ii = map_.find(block_id);
            if (ii != map_.end()) {
                ii->second.value += value;

                if (ii->second.value == 0)
                {                                        
                    map_.erase(ii);
                }
            }
            else if (value > 0) {                
                map_[block_id] = Counter{value};
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Negative new counter value for block {} :: {}", block_id, value);
            }
        }
    }

    void clear() noexcept {
        map_.clear();
    }

    auto begin() const noexcept {
        return map_.begin();
    }

    auto end() const noexcept {
        return map_.end();
    }

    size_t size() const noexcept {
        return map_.size();
    }

    bool add_root(const BlockID& block_id)
    {
        auto ii = ctr_roots_.find(block_id);
        if (ii != ctr_roots_.end()) {
            return false;
        }
        else {
            ctr_roots_.insert(block_id);
            return true;
        }
    }

    void set(const BlockID& block_id, int64_t counter) noexcept {
        map_[block_id] = Counter{counter};
    }

    int cnt_{};

    bool inc(const BlockID& block_id) noexcept
    {
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

    bool dec(const BlockID& block_id)
    {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            bool res = ii->second.dec();
            if (res) {
                map_.erase(ii);
            }
            return res;
        }
        else {
            make_generic_error_with_source(MA_SRC, "SWMRStore block counter is not found for block {}", block_id).do_throw();
        }
    }

    void for_each(std::function<void (const BlockID&, int64_t)> fn) const noexcept
    {
        for (auto ii = map_.begin(); ii != map_.end(); ++ii) {
            fn(ii->first, ii->second.value);
        }
    }

    Optional<int64_t> get(const BlockID& block_id) const noexcept {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            return ii->second.value;
        }
        return Optional<int64_t>{};
    }

    void diff(const SWMRBlockCounters& other) const noexcept
    {
        for (const auto& entry: map_)
        {
            auto val = other.get(entry.first);
            if (!val) {
                println("Counter for block {} is missing in the second map", entry.first);
            }
            else if (val.get() != entry.second.value) {
                println("Counter value mismatch for block {}. Expected {}, actual {}", entry.first, entry.second.value, val.get());
            }
        }

        for (const auto& entry: other.map_)
        {
            auto val = get(entry.first);
            if (!val) {
                println("Counter for block {} is missing in the first map", entry.first);
            }
            else if (val.get() != entry.second.value) {
                println("Counter value mismatch for block {}. Expected {}, actual {}", entry.first, val.get(), entry.second.value);
            }
        }
    }
};


template <typename Profile>
class SWMRCounterBlock {
    using CounterStorageT = CounterStorage<Profile>;
    uint64_t next_block_pos_;
    uint64_t size_;
    uint64_t capacity_;
    CounterStorageT counters_[1];
public:
    void init(int32_t block_size) noexcept
    {
        capacity_ = capacity_for(block_size);
        size_ = 0;
        next_block_pos_ = 0;
    }

    uint64_t size() const noexcept {
        return size_;
    }

    uint64_t next_block_pos() const noexcept {
        return next_block_pos_;
    }

    void set_next_block_pos(uint64_t val) noexcept {
        next_block_pos_ = val;
    }

    Span<CounterStorageT> counters() noexcept {
        return Span<CounterStorageT>{counters_, size_};
    }

    Span<const CounterStorageT> counters() const noexcept {
        return Span<const CounterStorageT>{counters_, size_};
    }

    bool add_counter(const CounterStorageT& cnt) noexcept
    {
        if (size_ < capacity_) {
            counters_[size_++] = cnt;
            return true;
        }
        return false;
    }

    uint64_t capacity() const noexcept {
        return capacity_;
    }

    uint64_t available() const noexcept {
        return capacity_ - size_;
    }

    static uint64_t capacity_for(int32_t block_size) noexcept
    {
        return (block_size - offsetof(SWMRCounterBlock, counters_)) / sizeof(CounterStorageT);
    }
};


}
