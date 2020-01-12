
// Copyright 2015 Victor Smirnov
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

#include "../../prototype/bt/bt_test_base.hpp"

#include <memoria/v1/api/db/update_log/update_log_api.hpp>

#include <set>
#include <map>
#include <vector>
#include <queue>


namespace memoria {
namespace v1 {
namespace tests {



class UpdateLogTestBase: public BTTestBase<UpdateLog, InMemAllocator<>, DefaultProfile<>> {
public:
    using Base   = BTTestBase<UpdateLog, InMemAllocator<>, DefaultProfile<>>;

    using typename Base::Ctr;
    using typename Base::CtrName;

    using Base::branch;
    using Base::commit;
    using Base::allocator;
    using Base::check;
    using Base::out;
    using Base::getRandom;

    UUID ctr_name_{};

    size_t max_values_{30000000};
    size_t max_keys_fraction_{1000};
    size_t check_steps_{10};

    using CommandData = std::vector<uint8_t>;

    using SampleDataSorted          = std::map<UUID, CommandData>;
    using SampleDataShuffled        = std::vector<std::pair<UUID, CommandData>>;

    SampleDataSorted samples_;
    SampleDataShuffled snapshot_;

    MMA1_STATE_FILEDS(ctr_name_, max_values_, max_keys_fraction_, check_steps_)
    MMA1_INDIRECT_STATE_FILEDS(samples_, snapshot_)

    UpdateLogTestBase(){}

    std::vector<uint8_t> make_random_data(size_t size)
    {
        std::vector<uint8_t> data;
        for (size_t c = 0; c < size; c++)
        {
            data.push_back(getRandom(std::numeric_limits<uint8_t>::max()));
        }

        return data;
    }

    SampleDataSorted create_sample_data() {
        return create_sample_data(max_values_, max_keys_fraction_);
    }

    SampleDataSorted create_sample_data(size_t max_values, size_t max_keys_fraction)
    {
        size_t max_keys = max_values / max_keys_fraction;
        if (max_keys == 0) {
            max_keys = 1;
        }

        SampleDataSorted data;

        if (max_keys > 1)
        {
            size_t values = max_values_;
            size_t values_per_key = max_values_ / max_keys;

            for (size_t c = 0; c < max_keys; c++)
            {
                size_t values_size = getRandom(values_per_key * 2);

                UUID key = UUID::make_random();

                if (c < max_keys - 1)
                {
                    size_t values_to_insert = values_size <= values ? values_size : values;

                    data[key] = make_random_data(values_to_insert);

                    values -= values_to_insert;
                }
                else {
                    data[key] = make_random_data(values);
                }
            }
        }
        else {
            UUID key = UUID::make_random();
            data[key] = make_random_data(max_values_);
        }

        return data;
    }

    SampleDataShuffled shuffle(const SampleDataSorted& sorted_data)
    {
        SampleDataShuffled big_data;

        for (const auto& item: sorted_data)
        {
            big_data.push_back(std::make_pair(item.first, std::move(item.second)));
        }

        std::random_shuffle(big_data.begin(), big_data.end(), getGlobalInt64Generator());

        std::vector<std::queue<std::pair<UUID, CommandData>>> chunks{big_data.size()};

        for (size_t c = 0; c < big_data.size(); c++)
        {
            split_data_to(big_data[c], chunks[c]);
        }

        SampleDataShuffled data;

        bool has_chunks{};
        do
        {
            has_chunks = false;
            for (size_t c = 0; c < chunks.size(); c++)
            {
                if (chunks[c].size() > 0)
                {
                    auto ee = std::move(chunks[c].front());
                    data.push_back(std::make_pair(ee.first, std::move(ee.second)));
                    chunks[c].pop();
                    has_chunks = true;
                }
            }
        }
        while(has_chunks);

        return data;
    }

    void split_data_to(const std::pair<UUID, CommandData>& pair, std::queue<std::pair<UUID, CommandData>>& queue)
    {
        size_t pos{};

        const auto& data = pair.second;

        while (pos < data.size())
        {
            size_t next = getRandom(data.size() - pos + 1) + pos;

            queue.push(std::make_pair(pair.first, CommandData{std::vector<uint8_t>{data.begin() + pos, data.begin() + next}}));

            pos = next;
        }
    }

    void check_data(const std::vector<uint8_t>& exp, const std::vector<uint8_t>& act)
    {
        assert_equals(exp.size(), act.size());
        for (size_t c = 0; c < exp.size(); c++)
        {
            assert_equals(exp[c], act[c], "c = {}", c);
        }
    }

    template <typename Ctr, typename SampleDataSorted>
    void check_values(Ctr& ctr, const SampleDataSorted& data, size_t ccc = 0)
    {
        auto snp_ii = ctr.latest_snapshot();
        auto data_ii = data.begin();

        assert_equals(data.size(), (size_t)snp_ii.containers_size());

        auto ctr_ii = snp_ii.containers();

        while (ctr_ii.has_next())
        {
            auto exp_data_key = data_ii->first;
            const auto& exp_cmd_data = data_ii->second;

            auto act_cmd_data = ctr_ii.commands().as_vector();

            check_data(exp_cmd_data, act_cmd_data);

            auto ctr_name = ctr_ii.next();

            assert_equals(exp_data_key, ctr_name);

            data_ii++;
        }
    }
};


}}}
