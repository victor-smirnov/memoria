
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/result.hpp>

#include <boost/intrusive/list.hpp>

#include <unordered_map>
#include <iostream>

namespace memoria {

template <typename ID, typename EntryBase>
class TwoQueueCache {
    using MemberHookT = boost::intrusive::list_member_hook<
        boost::intrusive::link_mode<
            boost::intrusive::safe_link
        >
    >;

public:

    enum class QueueType {
        A1_In, A1_Out, Am
    };

    struct EntryT: EntryBase {
        MemberHookT list_hook;

        QueueType queue_type;
        size_t size;

        template <typename... Args>
        EntryT(Args&&... args) noexcept :
            EntryBase(std::forward<Args>(args)...),
            list_hook(),
            queue_type {QueueType::A1_In},
            size(0)
        {}

        bool is_linked() const noexcept {
            return list_hook.is_linked();
        }
    };

    using EvictionFn = std::function<void (bool, EntryT*)>;

private:
    using QueueT = boost::intrusive::list<
        EntryT,
        boost::intrusive::member_hook<
            EntryT,
            MemberHookT,
            &EntryT::list_hook
        >
    >;

    size_t am_capacity_;
    size_t a1_capacity_;
    size_t a1_out_capacity_{1024*1024};

    QueueT am_queue_;
    QueueT a1_in_queue_;
    QueueT a1_out_queue_;
    std::unordered_map<ID, EntryT*> map_;

    EvictionFn eviction_fn_;

public:
    TwoQueueCache(size_t am_capacity, EvictionFn eviction_fn) noexcept :
        am_capacity_(am_capacity), a1_capacity_(am_capacity / 4),
        eviction_fn_(eviction_fn)
    {}

    TwoQueueCache(size_t am_capacity, size_t a1_capacity, EvictionFn eviction_fn) noexcept :
        am_capacity_(am_capacity), a1_capacity_(a1_capacity),
        eviction_fn_(eviction_fn)
    {}

    size_t map_size() const noexcept {
        return map_.size();
    }

    size_t am_queue_size() const noexcept {
        return am_queue_.size();
    }

    size_t a1_in_queue_size() const noexcept {
        return a1_in_queue_.size();
    }

    size_t a1_out_queue_size() const noexcept {
        return a1_out_queue_.size();
    }

    Optional<EntryT*> get(const ID& id) noexcept {
        auto ii = map_.find(id);
        if (ii != map_.end())
        {
            EntryT* entry = ii->second;

            if (ii->second->is_linked()) {
                if (entry->queue_type == QueueType::A1_In) {
                    a1_in_queue_.erase(a1_in_queue_.iterator_to(*entry));
                    entry->queue_type = QueueType::Am;
                }
                else if (entry->queue_type == QueueType::A1_Out) {
                    a1_out_queue_.erase(a1_out_queue_.iterator_to(*entry));
                    entry->queue_type = QueueType::Am;
                }
                else {
                    am_queue_.erase(am_queue_.iterator_to(*entry));
                }
            }

            return Optional<EntryT*>{entry};
        }

        return Optional<EntryT*>{};
    }

    Optional<EntryT*> has_entry(const ID& id) noexcept {
        auto ii = map_.find(id);
        if (ii != map_.end())
        {
            EntryT* entry = ii->second;
            return Optional<EntryT*>{entry};
        }

        return Optional<EntryT*>{};
    }


    void insert(EntryT* new_entry) noexcept {
        map_.insert(std::make_pair(new_entry->id(), new_entry));
    }

    void attach(EntryT* entry)
    {
        if (entry->queue_type == QueueType::A1_In)
        {
            while (a1_in_queue_.size() > a1_capacity_) {
                auto ii = a1_in_queue_.begin();
                EntryT* evicting = &*ii;

                a1_in_queue_.erase(ii);

                a1_out_queue_.push_back(*evicting);

                eviction_fn_(true, evicting);
            }

            a1_in_queue_.push_back(*entry);
        }
        else if (entry->queue_type == QueueType::A1_Out)
        {
            while (a1_out_queue_.size() > a1_out_capacity_) {
                auto ii = a1_out_queue_.begin();
                EntryT* evicting = &*ii;
                map_.erase(evicting->id());
                a1_out_queue_.erase(ii);

                eviction_fn_(false, evicting);
            }

            a1_out_queue_.push_back(*entry);
        }
        else {
            while (am_queue_.size() > am_capacity_) {
                auto ii = am_queue_.begin();
                am_queue_.erase(ii);
                map_.erase((*ii).id());
                eviction_fn_(false, &*ii);
            }
        }
    }

    Optional<EntryT*> remove(const ID& id) noexcept {
        auto entry = get(id);

        if (entry) {
            map_.erase(entry.value()->id());
        }

        return entry;
    }

    void remove_from_map(const ID& id) noexcept {
        map_.erase(id);
    }

    template <typename Fn>
    void for_each_entry(Fn&& fn) {
        for (auto entry: map_) {
            fn(entry.second);
        }
    }
};

}
