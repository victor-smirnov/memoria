
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

#include <memoria/api/allocation_map/allocation_map_api.hpp>
#include <memoria/core/tools/arena_buffer.hpp>

#include <list>

namespace memoria {


template <typename Profile, int32_t Levels>
class AllocationPoolData {
    static_assert (Levels > 1, "");

    static constexpr uint32_t LEVEL_CAPACITY = 4;
    static constexpr uint32_t LEVEL0_CAPACITY = 64;

    using BlkAlloc  = BasicBlockAllocation<Profile>;
    using AllocMeta = AllocationMetadata<Profile>;

    using BlkAllocArray = BlkAlloc[LEVEL_CAPACITY];

    BlkAllocArray levels_[Levels - 1];
    BlkAlloc level0_[LEVEL0_CAPACITY];
    uint32_t level_size_[Levels];
public:
    AllocationPoolData()  {
        clear();
    }

    void preconfigure(const AllocationPoolData<Profile, Levels>& sample)
    {
        clear();

        for (size_t level = 0; level < (size_t)Levels; level++){
            levels_[level].ensure(sample.capacity(level));
        }
    }

    void clear()
    {
        for (auto& arr: levels_) {
            for (auto& alc: arr) {
                alc = BlkAlloc{};
            }
        }

        for (auto& alc: level0_) {
            alc = BlkAlloc{};
        }

        for (auto& ii: level_size_) {
            ii = 0;
        }
    }

    Span<BlkAlloc> span(size_t level)  {
        if (level == 0) {
            return Span<BlkAlloc> {level0_, level_size_[0]};
        }
        else {
            return Span<BlkAlloc> {levels_[level - 1], level_size_[level]};
        }
    }

    Span<const BlkAlloc> span(size_t level) const  {
        if (level == 0) {
            return Span<const BlkAlloc> {level0_, level_size_[0]};
        }
        else {
            return Span<const BlkAlloc> {levels_[level - 1], level_size_[level]};
        }
    }

    const BlkAlloc* data(size_t level) const  {
        if (level == 0) {
            return level0_;
        }
        else {
            return levels_[level - 1];
        }
    }

    BlkAlloc* data(size_t level)  {
        if (level == 0) {
            return level0_;
        }
        else {
            return levels_[level - 1];
        }
    }

    size_t capacity(size_t level) const  {
        if (level == 0) {
            return LEVEL0_CAPACITY;
        }
        else {
            return LEVEL_CAPACITY;
        }
    }

    size_t size(size_t level) const  {
        return level_size_[level];
    }

    void fill(size_t level, Span<const AllocMeta> span)
    {
        level_size_[level] = span.size();

        if (level == 0) {
            for (size_t c = 0; c < span.size(); c++) {
                level0_[c] = span[c];
            }
        }
        else {
            for (size_t c = 0; c < span.size(); c++) {
                levels_[level - 1][c] = span[c];
            }
        }
    }

    template <typename II>
    void fill(size_t level, II iter, II end)
    {
        level_size_[level] = 0;

        if (level == 0) {
            for (size_t c = 0; iter != end; ++iter, ++c) {
                level0_[c] = *iter;
                ++level_size_[0];
            }
        }
        else {
            for (size_t c = 0; iter != end; ++iter, ++c) {
                levels_[level - 1][c] = *iter;
                ++level_size_[level];
            }
        }
    }

};


template <typename Profile>
class AllocationPoolData<Profile, 1> {
    static constexpr uint32_t LEVEL0_CAPACITY = 64;

    using BlkAlloc  = BasicBlockAllocation<Profile>;
    using AllocMeta = AllocationMetadata<Profile>;

    BlkAlloc level0_[LEVEL0_CAPACITY];
    uint32_t level_size_[1];

public:
    AllocationPoolData()  {
        clear();
    }

    void clear()
    {
        for (auto& alc: level0_) {
            alc = BlkAlloc{};
        }

        for (auto& ii: level_size_) {
            ii = 0;
        }
    }

    Span<BlkAlloc> span(size_t level)  {
        return Span<BlkAlloc> {level0_, level_size_[0]};
    }

    Span<const BlkAlloc> span(size_t level) const  {
        return Span<BlkAlloc> {level0_, level_size_[0]};
    }

    const BlkAlloc* data(size_t level) const  {
        return level0_;
    }

    BlkAlloc* data(size_t level)  {
        return level0_;
    }

    size_t capacity(size_t level) const  {
        return LEVEL0_CAPACITY;
    }

    size_t size(size_t level) const  {
        return level_size_[0];
    }

    void fill(size_t level, Span<const AllocMeta> span)
    {
        level_size_[0] = span.size();
        for (size_t c = 0; c < span.size(); c++) {
            level0_[c] = span[c];
        }
    }

    template <typename II>
    void fill(size_t level, II iter, II end)
    {
        level_size_[0] = 0;
        for (size_t c = 0; iter != end; ++iter, ++c) {
            level0_[c] = *iter;
            ++level_size_[0];
        }
    }
};



template <typename Profile, int32_t Levels>
class AllocationPool {
    using AlcMetadata = AllocationMetadata<Profile>;
    using PoolData = AllocationPoolData<Profile, Levels>;

    class LevelQueue {
        int64_t total_;
        size_t capacity_;
        std::list<AlcMetadata> buffer_;

    public:
        void init(size_t capacity) {
            capacity_ = capacity;
            clear();
        }

        void clear()  {
            total_ = 0;
            buffer_.clear();
        }

        bool is_empty() const  {
            return buffer_.size() == 0;
        }

        size_t size() const  {
            return buffer_.size();
        }

        size_t capacity() const  {
            return capacity_;
        }

        int64_t total() const  {
            return total_;
        }

        void add_total(int64_t amt)  {
            total_ += amt;
        }

        bool push(const AlcMetadata& meta)
        {
            if (size() < capacity())
            {
                buffer_.push_back(meta);
                total_ += meta.size_at_level();
                return true;
            }
            else {
                return false;
            }
        }

        AlcMetadata allocate_one()
        {
            AlcMetadata& tail = buffer_.front();
            AlcMetadata meta = tail.take(1);
            --total_;
            if (tail.size1() == 0) {
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
    };

    LevelQueue levels_[Levels];

    int64_t level0_total_{};
    int64_t level0_reserved_{32};

public:
    AllocationPool()
    {
        PoolData sample;

        for (int32_t c = 0; c < Levels; c++)
        {
            levels_[c].init(sample.capacity(c));
        }
    }

    size_t available_capacity(int32_t level) const  {
        return levels_[level].capacity() - levels_[level].size();
    }

    int64_t level0_total() const  {
        return level0_total_;
    }

    int64_t level0_reserved() const  {
        return level0_reserved_;
    }

    void load(const PoolData& data)
    {
        for (int32_t level = 0; level < Levels; level++)
        {
            levels_[level].init(data.capacity(level));
            auto span = data.span(level);
            for (const auto& alc: span) {
                auto meta = AlcMetadata{alc, level};
                if (!add(meta)) {
                    println("Internal error. Can't load allocation pool metadata from the superblock.");
                }
            }
        }
    }

    void store(PoolData& data) const
    {
        for (size_t level = 0; level < (size_t)Levels; level++)
        {
            data.fill(level, levels_[level].cbegin(), levels_[level].cend());
        }
    }

    bool add(const AlcMetadata& meta)
    {
        if (levels_[meta.level()].push(meta))
        {
            level0_total_ += meta.size1();
            return true;
        }

        return false;
    }

    bool add(int64_t position, int64_t size, int32_t level)
    {
        return add(AlcMetadata(position, size, level));
    }

    void clear() {
        for (int32_t ll = 0; ll < Levels; ll++)
        {
            levels_[ll].clear();
        }

        level0_total_ = 0;
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



    Optional<AlcMetadata> allocate_one(int32_t level)
    {
        int64_t l0_size = static_cast<int64_t>(1) << level;
        if (level0_total_ - level0_reserved_ >= l0_size ) {
            return do_allocate_one(level);
        }

        return Optional<AlcMetadata>{};
    }

    Optional<AlcMetadata> allocate_reserved(int64_t remainder)
    {
        if (level0_total_ > remainder) {
            return do_allocate_one(0);
        }

        return Optional<AlcMetadata>{};
    }

    bool has_room(int32_t level) const
    {
        int64_t ll_capacity = levels_[level].capacity();
        int64_t ll_total = compute_level_total(level);
        return ll_total < ll_capacity;
    }

    void for_each(const std::function<void (const AlcMetadata& meta)>& fn) const
    {
        for (size_t ll = 0; ll < Levels; ll++) {
            for (const AlcMetadata& meta: levels_[ll]) {
                fn(meta);
            }
        }
    }


private:
    int64_t compute_level_total(int32_t level) const
    {
        int64_t total = 0;
        for (int32_t c = level; c < Levels; c++) {
            total += levels_[c].total() << (c - level);
        }

        return total;
    }

    Optional<AlcMetadata> do_allocate_one(int32_t level)
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

        return Optional<AlcMetadata>{};
    }

    void borrow_from_above(int32_t level)
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
