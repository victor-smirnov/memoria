
// Copyright 2018 Victor Smirnov
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
#include "edge_map_values_test_base.hpp"

#include <memoria/api/db/edge_map/edge_map_api.hpp>

#include <set>
#include <map>
#include <vector>

namespace memoria {
namespace tests {

class EdgeMapValuesInsertionTestSuite: public EdgeMapValuesRemovalTestBase {
public:
    using Base   = EdgeMapValuesRemovalTestBase;
    using MyType = EdgeMapValuesInsertionTestSuite;

    using SampleDataShuffledByKey   = std::vector<std::pair<UUID, std::set<UAcc128T>>>;
    SampleDataShuffledByKey snapshot_by_key_;

    MMA_INDIRECT_STATE_FILEDS(snapshot_by_key_)

    EdgeMapValuesInsertionTestSuite(){}

    static void init_suite(TestSuite& suite)
    {
        MMA_CLASS_TEST_WITH_REPLAY(suite, testInsertSingleKeyValue, replayInsertSingleKeyValue);
        MMA_CLASS_TEST_WITH_REPLAY(suite, testInsertMultipleKeyValues, replayInsertMultipleKeyValues);
    }

    virtual void post_configure(TestCoverage coverage)
    {
        max_values_ = select_for_coverage<size_t>(
            coverage,
            1000,
            10000,
            100000,
            1000000,
            10000000,
            100000000
        );

        max_keys_fraction_ = select_for_coverage<size_t>(
            coverage,
            100,
            100,
            1000,
            10000,
            10000,
            100000
        );
    }

    SampleDataShuffledByKey shuffle_keys(const SampleDataSorted& sorted_data)
    {
        SampleDataShuffledByKey data;

        for (const auto& item: sorted_data)
        {
            data.push_back(std::make_pair(item.first, item.second));
        }

        std::random_shuffle(data.begin(), data.end(), getGlobalInt64Generator());
        return data;
    }


    template <typename Ctr>
    void insert_pair(Ctr& ctr, SampleDataSorted& samples, const UUID& key, const Optional<UAcc128T>& value)
    {
        if (value)
        {
            bool result = ctr.upsert(key, value.value());
            assert_equals(true, result);
            samples[key].insert(value.value());
        }
        else {
            samples[key] = std::set<UAcc128T>();
            auto result = ctr.find_or_create(key);
            assert_equals(true, result.is_found(key));
        }
    }



    void testInsertSingleKeyValue()
    {
        auto sample_data = create_sample_data();
        const auto shuffled_samples = shuffle(sample_data);

        auto snp = branch();

        ctr_name_ = create<CtrName>(snp).name();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        auto t0 = getTimeInMillis();

        size_t cnt = 0;
        for (const auto& kv_pair: shuffled_samples)
        {
            if (cnt % (max_values_ / check_steps_) == 0)
            {
                out() << "Inserted " << cnt << " k/v pairs" << std::endl;
            }

            auto key    = kv_pair.first;
            auto value  = kv_pair.second;

            snapshot_.push_back(std::make_pair(key, value));

            insert_pair(ctr, samples_, key, value);

            if (cnt % (max_values_ / check_steps_) == 0)
            {
                check_values(ctr, samples_);

                commit();
                snp = branch();
                ctr  = find<CtrName>(snp, ctr_name_);
                snapshot_.clear();
            }

            cnt++;
        }

        out() << "Values inserted in " << FormatTime(getTimeInMillis() - t0) << std::endl;

        check_values(ctr, samples_);

        commit();
    }


    void replayInsertSingleKeyValue()
    {
        auto snp = branch();

        auto ctr  = find<CtrName>(snp, ctr_name_);

        for (const auto& kv_pair: snapshot_)
        {
            auto key = kv_pair.first;
            auto value  = kv_pair.second;

            if (value) {
                samples_[key].erase(value.value());
            }
            else {
                samples_.erase(key);
            }
        }

        for (const auto& kv_pair: snapshot_)
        {
            auto key    = kv_pair.first;
            auto value  = kv_pair.second;

            out() << "Insert pair: " << key << " :: " << (value ? value.value().to_bmp().str() : std::string("<none>")) << std::endl;

            insert_pair(ctr, samples_, key, value);

            check_values(ctr, samples_);
        }

        check_values(ctr, samples_);
        commit();
    }





    void testInsertMultipleKeyValues()
    {
        auto sample_data = create_sample_data();
        auto shuffled_samples = shuffle_keys(sample_data);

        auto snp = branch();

        ctr_name_ = create<CtrName>(snp).name();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        commit();
        snp = branch();
        ctr  = find<CtrName>(snp, ctr_name_);

        auto t0 = getTimeInMillis();

        size_t cnt{};
        for (const auto& kv_pair: shuffled_samples)
        {
            auto key = kv_pair.first;
            const auto& values = kv_pair.second;

            snapshot_by_key_.push_back(std::make_pair(key, values));
            samples_[key] = values;

            insert_single_batch(ctr, key, values);

            if (cnt % (max_values_ / 10) == 0)
            {
                check_values(ctr, samples_);

                commit();
                snp = branch();
                ctr  = find<CtrName>(snp, ctr_name_);
                snapshot_by_key_.clear();
            }

            cnt++;
        }

        check_values(ctr, sample_data);

        out() << "Values inserted in " << FormatTime(getTimeInMillis() - t0) << std::endl;
    }



    void replayInsertMultipleKeyValues()
    {
        auto snp = branch();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        for (auto& vv: snapshot_by_key_)
        {
            samples_.erase(vv.first);
        }

        auto t0 = getTimeInMillis();

        for (const auto& kv_pair: snapshot_by_key_)
        {
            auto key = kv_pair.first;
            const auto& values = kv_pair.second;

            samples_[key] = values;

            insert_single_batch(ctr, key, values);

            check_values(ctr, samples_);
        }

        out() << "Values inserted in " << FormatTime(getTimeInMillis() - t0) << std::endl;
    }
};



namespace {
auto Suite1 = register_class_suite<EdgeMapValuesInsertionTestSuite>("EdgeMap");
}

}}
