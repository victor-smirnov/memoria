
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

#include <memoria/core/flat_map/flat_hash_map.hpp>

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
        auto inc()  {
            return ++value;
        }

        bool dec()  {
            return --value == 0;
        }
    };

private:
//    absl::btree_set<BlockID> ctr_roots_;
//    absl::btree_map<BlockID, Counter> map_;
//    ska::flat_hash_set<BlockID> ctr_roots_;
    ska::flat_hash_map<BlockID, Counter> map_;

//    std::unordered_set<BlockID> ctr_roots_;
//    std::unordered_map<BlockID, Counter> map_;

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

                //println("ApplyCtrBlock: {}::{}::{}", block_id, value, ii->second.value);

                if (ii->second.value == 0)
                {                                        
                    map_.erase(ii);
                }
            }
            else if (value > 0) {
                //println("ApplyNewCtrBlock: {}::{}", block_id, value);
                map_[block_id] = Counter{value};
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Negative new counter value for block {} :: {}", block_id, value);
            }
        }
    }

    void clear()  {
        map_.clear();
    }

    auto begin() const  {
        return map_.begin();
    }

    auto end() const  {
        return map_.end();
    }

    size_t size() const  {
        return map_.size();
    }

//    bool has_root(const BlockID& block_id) const {
//        auto ii = ctr_roots_.find(block_id);
//        return ii != ctr_roots_.end();
//    }

//    bool add_root(const BlockID& block_id)
//    {
//        println("Adding ctrRoot: {}", block_id);

//        auto ii = ctr_roots_.find(block_id);
//        if (ii != ctr_roots_.end()) {
//            return false;
//        }
//        else {
//            ctr_roots_.insert(block_id);
//            return true;
//        }
//    }

    void set(const BlockID& block_id, int64_t counter)  {
        map_[block_id] = Counter{counter};
    }

    int cnt_{};

    bool inc(const BlockID& block_id)
    {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            auto vv = ii->second.inc();

            //println("IncCtr: {}::{}", block_id, vv);

            return false;
        }
        else {
            map_.insert(std::make_pair(block_id, Counter{1}));
            //println("IncCtr: {}::{}", block_id, 1);

            return true;
        }
    }

    bool dec(const BlockID& block_id)
    {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            bool res = ii->second.dec();
            if (res) {
                //println("DecCtr: {}::{}", block_id, 0);
                map_.erase(ii);
            }
            else {
                //println("DecCtr: {}::{}", block_id, ii->second.value);
            }

            return res;
        }
        else {
            make_generic_error_with_source(MA_SRC, "SWMRStore block counter is not found for block {}", block_id).do_throw();
        }
    }

    void for_each(std::function<void (const BlockID&, int64_t)> fn) const
    {
        for (auto ii = map_.begin(); ii != map_.end(); ++ii) {
            fn(ii->first, ii->second.value);
        }
    }

    Optional<int64_t> get(const BlockID& block_id) const  {
        auto ii = map_.find(block_id);
        if (ii != map_.end()) {
            return ii->second.value;
        }
        return Optional<int64_t>{};
    }

    void diff(const SWMRBlockCounters& other) const
    {
        for (const auto& entry: map_)
        {
            auto val = other.get(entry.first);
            if (!val) {
                println("Counter for block {} is missing in the computed map", entry.first);
            }
            else if (val.get() != entry.second.value) {
                println("Counter value mismatch for block {}. Expected {}, actual {}", entry.first, entry.second.value, val.get());
            }
        }

        for (const auto& entry: other.map_)
        {
            auto val = get(entry.first);
            if (!val) {
                println("Counter for block {} is missing in the loaded map", entry.first);
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
    void init(int32_t block_size)
    {
        capacity_ = capacity_for(block_size);
        size_ = 0;
        next_block_pos_ = 0;
    }

    uint64_t size() const  {
        return size_;
    }

    uint64_t next_block_pos() const  {
        return next_block_pos_;
    }

    void set_next_block_pos(uint64_t val)  {
        next_block_pos_ = val;
    }

    Span<CounterStorageT> counters()  {
        return Span<CounterStorageT>{counters_, size_};
    }

    Span<const CounterStorageT> counters() const  {
        return Span<const CounterStorageT>{counters_, size_};
    }

    bool add_counter(const CounterStorageT& cnt)
    {
        if (size_ < capacity_) {
            counters_[size_++] = cnt;
            return true;
        }
        return false;
    }

    uint64_t capacity() const  {
        return capacity_;
    }

    uint64_t available() const  {
        return capacity_ - size_;
    }

    static uint64_t capacity_for(int32_t block_size)
    {
        return (block_size - offsetof(SWMRCounterBlock, counters_)) / sizeof(CounterStorageT);
    }
};


}
