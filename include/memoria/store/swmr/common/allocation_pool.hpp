
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

namespace memoria {



template <typename Profile, int32_t Levels>
class AllocationPool {
    using AlcMetadata = AllocationMetadata<Profile>;

    ArenaBuffer<AlcMetadata> buffer_;

    int64_t totals_[Levels];
    ArenaBuffer<AlcMetadata> levels_[Levels];

public:
    AllocationPool() noexcept {
        for (int32_t c = 0; c < Levels; c++) {
            totals_[c] = 0;
        }
    }

    void add(const AlcMetadata& meta) noexcept
    {
        levels_[meta.level()].append_value(meta);
        totals_[meta.level()] += meta.size();
    }

    Optional<AlcMetadata> allocate_one(int32_t level) noexcept
    {
        buffer_.clear();
        allocate(level, 1, buffer_);

        if (buffer_.size() > 0) {
            return buffer_.tail();
        }
        else {
            return Optional<AlcMetadata>{};
        }
    }

    void allocate(int32_t level, int64_t amount, ArenaBuffer<AlcMetadata>& buffer) noexcept
    {
        if (totals_[level] < amount)
        {
            if (!populate_level_from_above(level, amount - totals_[level])) {
                return;
            }
        }

        size_t cnt{};
        int64_t sum{};
        for (auto& alc: levels_[level].span())
        {
            if (alc.size() + sum <= amount) {
                sum += alc.size();
                totals_[level] -= alc.size();
                cnt++;
                buffer.append_value(alc);
            }
            else {
                int64_t remainder = amount - sum;
                sum = amount;
                buffer.append_value(alc.take(remainder));
                totals_[level] -= remainder;
                break;
            }
        }

        levels_[level].remove(0, cnt);
    }

    void clear() noexcept {
        for (int32_t ll = 0; ll < Levels; ll++)
        {
            levels_[ll].clear();
            totals_[ll] = 0;
        }
    }

    ArenaBuffer<AlcMetadata>& level_buffer(int32_t ll) noexcept {
        return levels_[ll];
    }

    void refresh(int32_t level) noexcept
    {
        totals_[level] = 0;
        for (const auto& alc: levels_[level].span()) {
            totals_[level] += alc.size();
        }
    }

    void dump(std::ostream& out = std::cout) const noexcept
    {
        out << "Allocation Pool :: Levels: " << Levels << std::endl;
        for (int c = 0; c < Levels; c++) {
            out << "total(" << c << "): " << totals_[c] << std::endl;
            for (const auto& alc: levels_[c].span()) {
                out << "    " << alc << std::endl;
            }
        }
    }

private:
    bool populate_level_from_above(int32_t level, int64_t amount) noexcept
    {
        int64_t available{};
        for (int32_t ll = level + 1; ll < Levels; ll++) {
            available += totals_[ll] << (ll - level);
        }

        if (available >= amount)
        {
            int64_t sum{};

            for (int32_t ll = level + 1; ll < Levels && sum < amount; ll++)
            {
                size_t cc;
                for (cc = 0; cc < levels_[ll].size(); cc++)
                {
                    int64_t ll0_remainder = amount - sum;

                    auto& avl_alc = levels_[ll][cc];
                    int64_t avl_size = avl_alc.size();
                    int64_t ll0_avl_size = avl_size << (ll - level);

                    if (ll0_avl_size <= ll0_remainder)
                    {
                        totals_[ll] -= avl_alc.size();

                        auto meta_ll = avl_alc.take_all_for(level);
                        levels_[level].append_value(meta_ll);
                        totals_[level] += meta_ll.size();

                        sum += ll0_avl_size;
                    }
                    else {
                        int64_t scale_mask = (1ll << (ll - level + 1)) - 1;
                        int64_t ll_remainder = (
                                    (ll0_remainder >> (ll - level)) +
                                    ((ll0_remainder & scale_mask) != 0)
                        );

                        auto meta_ll = avl_alc.take_for(ll_remainder, level);
                        levels_[level].append_value(meta_ll);
                        totals_[level] += meta_ll.size();

                        totals_[ll] -= ll_remainder;

                        if (avl_alc.size() == 0) {
                            ++cc;
                        }

                        break;
                    }
                }

                if (cc < levels_[ll].size()) {
                    levels_[ll].remove(0, cc);
                }
                else {
                    levels_[ll].clear();
                }
            }

            return true;
        }
        else {
            return false;
        }
    }
};

}
