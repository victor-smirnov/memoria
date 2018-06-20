
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

#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>

#include <set>
#include <map>
#include <vector>

namespace memoria {
namespace v1 {
namespace tests {

class EdgeMapValuesRemovalTestBase: public BTTestBase<EdgeMap, InMemAllocator<>, DefaultProfile<>> {
public:
    using Base   = BTTestBase<EdgeMap, InMemAllocator<>, DefaultProfile<>>;

    using typename Base::Ctr;
    using typename Base::CtrName;

    using Base::branch;
    using Base::commit;
    using Base::allocator;
    using Base::check;
    using Base::out;

    UUID ctr_name_{};

    size_t max_values_{10000000};
    size_t max_keys_fraction_{1000};
    size_t check_steps_{10};

    using SampleDataSorted          = std::map<UUID, std::set<UAcc128T>>;
    using SampleDataShuffled        = std::vector<std::pair<UUID, Optional<UAcc128T>>>;

    SampleDataSorted samples_;

    SampleDataShuffled snapshot_;

    MMA1_STATE_FILEDS(ctr_name_, max_values_, max_keys_fraction_, check_steps_)
    MMA1_INDIRECT_STATE_FILEDS(samples_, snapshot_)

    EdgeMapValuesRemovalTestBase(){}

    std::set<UAcc128T> make_random_values(size_t size)
    {
        std::set<UAcc128T> values;
        for (size_t c = 0; c < size; c++)
        {
            UAcc128T sample_value{};
            sample_value.value_[0] = getBIRandom();
            sample_value.value_[1] = getBIRandom();

            values.insert(sample_value);
        }

        return values;
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

                    data[key] = make_random_values(values_to_insert);

                    values -= values_to_insert;
                }
                else {
                    data[key] = make_random_values(values);
                }
            }
        }
        else {
            UUID key = UUID::make_random();
            data[key] = make_random_values(max_values_);
        }

        return data;
    }

    SampleDataShuffled shuffle(const SampleDataSorted& sorted_data)
    {
        SampleDataShuffled data;

        for (const auto& item: sorted_data)
        {
            if (item.second.size() > 0)
            {
                for (const auto& value: item.second)
                {
                    data.push_back(std::make_pair(item.first, value));
                }
            }
            else {
                data.push_back(std::make_pair(item.first, Optional<UAcc128T>()));
            }
        }

        std::shuffle(data.begin(), data.end(), getGlobalInt64Generator());
        return data;
    }


    template <typename Ctr, typename SampleDataSorted>
    void check_values(Ctr& ctr, const SampleDataSorted& data)
    {
        auto keys_ii = ctr.keys();
        auto data_ii = data.begin();

        assert_equals(data.size(), (size_t)keys_ii.size());

        while (keys_ii.has_keys())
        {
            auto data_key = data_ii->first;
            const auto& data_values = data_ii->second;
            auto data_values_ii = data_values.begin();

            assert_equals(data_key, keys_ii.key());
            auto values_ii = keys_ii.values();

            assert_equals(data_values.size(), values_ii.size());

            while (values_ii.has_values())
            {
                auto value1 = *data_values_ii;
                auto value2 = values_ii.value();

                assert_equals(value1, value2);

                data_values_ii++;
                values_ii.next_value();
            }

            keys_ii.next_key();
            data_ii++;
        }
    }


    template <typename Ctr>
    void insert_single_batch(Ctr& ctr, UUID key, const std::set<UAcc128T>& values)
    {
        std::vector<UAcc128T> values_data;

        UAcc128T last{};
        for (auto value: values)
        {
            auto tmp = value;
            value -= last;
            values_data.push_back(value);
            last = tmp;
        }

        ctr.assign(key, values_data.begin(), values_data.end());
    }

};


}}}
