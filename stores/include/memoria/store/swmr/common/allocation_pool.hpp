
// Copyright 2020-2025 Victor Smirnov
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

#include <memoria/api/allocation_map/allocation_map_api.hpp>
#include <memoria/core/tools/arena_buffer.hpp>

#include <list>

namespace memoria {


template <size_t Size>
class AllocationPoolData {

    using DataT = uint64_t;

    uint64_t size_;
    DataT slots_[Size];

public:
    constexpr AllocationPoolData() {
        reset();
    }

    constexpr void reset()
    {
        for (auto& slot: slots_) {
            slot = {};
        }
        size_ = 0;
    }

    constexpr void push_back(DataT data) {
        slots_[size_++] = data;
    }

    constexpr Span<uint64_t> span() {
        return Span<DataT> {slots_, size_};
    }

    constexpr Span<const uint64_t> span() const {
        return Span<const DataT> {slots_, size_};
    }


    static constexpr size_t capacity() {
        return Size;
    }

    constexpr size_t size() const  {
        return size_;
    }
};




template <typename Profile, size_t Levels>
class AllocationPool {
    using AlcMetadata = AllocationMetadata<Profile>;

    using SizeT = typename AlcMetadata::SizeT;

    class LevelQueue {
        SizeT total_;
        std::list<AlcMetadata> buffer_;

    public:
        void init() {
            clear();
        }

        void clear()  {
            total_ = 0;
            buffer_.clear();
        }

        bool is_empty() const  {
            return buffer_.empty();
        }

        size_t size() const  {
            return buffer_.size();
        }

        SizeT total() const  {
            return total_;
        }

        void add_total(SizeT amt)  {
            total_ += amt;
        }

        void push_at(AlcMetadata* tgt, const AlcMetadata& meta)
        {
            tgt->enlarge1(meta.size_at_level());
            total_ += meta.size_at_level();
        }

        void push(const AlcMetadata& meta)
        {
            buffer_.push_back(meta);
            total_ += meta.size_at_level();
        }

        AlcMetadata* find_exising(const AlcMetadata& meta)
        {
            for (AlcMetadata& mm: buffer_) {
                if (mm.joinable_with(meta)) {
                    return &mm;
                }
            }
            return nullptr;
        }

        AlcMetadata allocate_one()
        {
            AlcMetadata& tail = buffer_.front();
            AlcMetadata meta = tail.take(1);
            --total_;
            if (tail.size_at_level() == 0) {
                buffer_.pop_front();
            }
            return meta;
        }

        auto begin()  {
            return buffer_.begin();
        }

        auto end()  {
            return buffer_.end();
        }

        auto begin() const  {
            return buffer_.cbegin();
        }

        auto end() const  {
            return buffer_.cend();
        }

        auto cbegin() const  {
            return buffer_.cbegin();
        }

        auto cend() const  {
            return buffer_.cend();
        }

        SizeT drain_to(std::vector<AlcMetadata>& buf)
        {
            SizeT l0_blocks = 0;
            for (const AlcMetadata& alc: buffer_) {
                l0_blocks += alc.size1();
                buf.push_back(alc);
            }
            clear();
            return l0_blocks;
        }
    };

    LevelQueue levels_[Levels];

    uint64_t level0_total_{};
    uint64_t level0_reserved_{32};

    size_t size_{};
    size_t capacity_{};

public:
    AllocationPool(size_t capacity):
        capacity_(capacity)
    {
        for (int32_t c = 0; c < Levels; c++) {
            levels_[c].init();
        }
    }

    size_t reserved(size_t level) const
    {
        size_t cnt = 0;
        for (size_t c = 0; c < Levels; c++) {
            if (MMA_LIKELY(c != level)) {
                cnt += levels_[c].is_empty();
            }
        }
        return cnt;
    }


    size_t available_slots(size_t level) const {
        return capacity_ - reserved(level);
    }

    uint64_t level0_total() const  {
        return level0_total_;
    }

    uint64_t level0_reserved() const  {
        return level0_reserved_;
    }

    template <size_t Size>
    void load(const AllocationPoolData<Size>& data)
    {
        for (auto raw: data.span()) {
            AlcMetadata meta = AlcMetadata::from_raw(raw);
            add(meta);
        }
    }

    template <size_t Size>
    void store(AllocationPoolData<Size>& data) const
    {
        data.reset();
        for (const LevelQueue& level: levels_) {
            for (const AlcMetadata& meta: level) {
                data.push_back(meta.raw_data());
            }
        }
    }

    bool add(const AlcMetadata& meta)
    {
        AlcMetadata* tgt = levels_[meta.level()].find_exising(meta);
        if (tgt)
        {
            if (tgt->fits(meta.size_at_level()))
            {
                levels_[meta.level()].push_at(tgt, meta);
                level0_total_ += meta.size1();
                return true;
            }
            else if (size_ < capacity_) {
                size_++;
                levels_[meta.level()].push(meta);
                level0_total_ += meta.size1();
                return true;
            }
            else {
                return false;
            }
        }
        else if (size_ < capacity_) {
            size_++;
            levels_[meta.level()].push(meta);
            level0_total_ += meta.size1();
            return true;
        }
        else {
            return false;
        }
    }

    bool add(int64_t position, int64_t size, int32_t level)
    {
        return add(AlcMetadata::from_l0(position, size, level));
    }

    void clear()
    {
        for (int32_t ll = 0; ll < Levels; ll++) {
            levels_[ll].clear();
        }

        level0_total_ = 0;
        size_ = 0;
    }

    void reset()  {
        clear();
    }

    void dump(std::ostream& out = std::cout) const
    {
        for_each([&](auto alc){
           out << alc << std::endl;
        });

        out << std::endl;
    }



    Optional<AlcMetadata> allocate_one(SizeT level)
    {
        SizeT l0_size = static_cast<SizeT>(1) << level;
        if (level0_total_ - level0_reserved_ >= l0_size ) {
            return do_allocate_one(level);
        }

        return {};
    }

    Optional<AlcMetadata> allocate_reserved(SizeT remainder)
    {
        if (level0_total_ > remainder) {
            return do_allocate_one(0);
        }

        return {};
    }


    void for_each(const std::function<void (const AlcMetadata& meta)>& fn) const
    {
        for (size_t ll = 0; ll < Levels; ll++) {
            for (const AlcMetadata& meta: levels_[ll]) {
                fn(meta);
            }
        }
    }

    bool has_room(size_t level) {
        return available_slots(level) > 0;
    }

    ArenaBuffer<AlcMetadata> drain(size_t level)
    {
        ArenaBuffer<AlcMetadata> buf;

        size_t total_alcs = 0;
        SizeT total_drained_l0 = 0;
        for (size_t c = 0; c < level; c++)
        {
           total_alcs += levels_[c].size();
           total_drained_l0 += levels_[c].drain_to(buf);
        }

        size_ -= total_alcs;
        level0_total_ -= total_drained_l0;

        return buf;
    }

private:
    SizeT compute_level_total(SizeT level) const
    {
        SizeT total = 0;
        for (SizeT c = level; c < Levels; c++) {
            total += levels_[c].total() << (c - level);
        }

        return total;
    }

    Optional<AlcMetadata> do_allocate_one(SizeT level)
    {
        if (MMA_UNLIKELY(levels_[level].is_empty())) {
            borrow_from_above(level);
        }

        if (!levels_[level].is_empty())
        {
            AlcMetadata alc = levels_[level].allocate_one();
            level0_total_ -= alc.size1();
            return alc;
        }

        return {};
    }

    void borrow_from_above(SizeT level)
    {
        if (level < Levels - 1) {
            if (MMA_UNLIKELY(levels_[level + 1].is_empty())) {
                borrow_from_above(level + 1);
            }

            if (!levels_[level + 1].is_empty())
            {
                AlcMetadata alc = levels_[level + 1].allocate_one();
                levels_[level].push(alc.as_level(level));
            }
        }
    }
};





}
