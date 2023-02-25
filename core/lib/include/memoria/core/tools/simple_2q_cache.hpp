
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

#include <boost/intrusive/list.hpp>

#include <unordered_map>
#include <iostream>

namespace memoria {

template <typename ID, typename EntryBase>
class SimpleTwoQueueCache {
    using MemberHookT = boost::intrusive::list_member_hook<
        boost::intrusive::link_mode<
            boost::intrusive::safe_link
        >
    >;

public:

    enum class QueueType {
        A1, Am
    };

    struct EntryT: EntryBase {
        MemberHookT list_hook;

        QueueType queue_type {QueueType::A1};

        template <typename... Args>
        EntryT(Args&&... args) noexcept :
            EntryBase(std::forward<Args>(args)...)
        {}

        bool is_linked() const noexcept {
            return list_hook.is_linked();
        }
    };

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
    QueueT am_queue_;
    QueueT a1_queue_;
    std::unordered_map<ID, EntryT*> map_;

public:
    SimpleTwoQueueCache(size_t am_capacity) noexcept :
        am_capacity_(am_capacity), a1_capacity_(am_capacity / 4)
    {}

    SimpleTwoQueueCache(size_t am_capacity, size_t a1_capacity) noexcept :
        am_capacity_(am_capacity), a1_capacity_(a1_capacity)
    {}

    size_t map_size() const noexcept {
        return map_.size();
    }

    size_t am_queue_size() const noexcept {
        return am_queue_.size();
    }

    size_t a1_queue_size() const noexcept {
        return a1_queue_.size();
    }

    Optional<EntryT*> get(const ID& id) noexcept {
        auto ii = map_.find(id);
        if (ii != map_.end())
        {
            EntryT* entry = ii->second;

            if (ii->second->is_linked()) {
                if (entry->queue_type == QueueType::A1) {
                    a1_queue_.erase(a1_queue_.iterator_to(*entry));
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


    void insert(EntryT* new_entry) noexcept {
        map_.insert(std::make_pair(new_entry->id(), new_entry));
    }


    template <typename Fn>
    void attach(EntryT* entry, Fn&& eviction_fn) noexcept
    {
        //std::cout << "Snapshot cache size: " << map_.size() << " :: " << am_queue_.size() << " :: " << a1_queue_.size() << std::endl;

        if (entry->queue_type == QueueType::A1)
        {
            while (a1_queue_.size() > a1_capacity_) {
                auto ii = a1_queue_.begin();
                a1_queue_.erase(ii);
                map_.erase((*ii).id());
                eviction_fn(&*ii);
            }

            a1_queue_.push_back(*entry);
        }
        else {
            while (am_queue_.size() > am_capacity_) {
                auto ii = am_queue_.begin();
                am_queue_.erase(ii);
                map_.erase((*ii).id());
                eviction_fn(&*ii);
            }
        }
    }

    Optional<EntryT*> remove(const ID& id) noexcept {
        auto entry = get(id);

        if (entry) {
            map_.erase(entry.get()->id());
        }

        return entry;
    }

    void remove_from_map(const ID& id) noexcept {
        map_.erase(id);
    }
};

}
