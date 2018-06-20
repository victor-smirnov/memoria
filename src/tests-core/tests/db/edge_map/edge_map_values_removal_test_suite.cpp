
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
#include "edge_map_values_test_base.hpp"


#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>



#include <set>
#include <map>
#include <vector>

namespace memoria {
namespace v1 {
namespace tests {

class EdgeMapValuesRemovalTestSuite: public EdgeMapValuesRemovalTestBase {
public:
    using Base   = EdgeMapValuesRemovalTestBase;
    using MyType = EdgeMapValuesRemovalTestSuite;

    using SampleDataShuffledByKey = std::vector<UUID>;
    SampleDataShuffledByKey snapshot_by_key_;

    MMA1_INDIRECT_STATE_FILEDS(snapshot_by_key_)

    EdgeMapValuesRemovalTestSuite(){}

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testRemoveSingleKeyValue, replayRemoveSingleKeyValue);
        MMA1_CLASS_TESTS(suite, testRemoveMultipleKeyValues);
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
            data.push_back(item.first);
        }

        std::shuffle(data.begin(), data.end(), getGlobalInt64Generator());
        return data;
    }


    template <typename Ctr>
    void remove_pair(Ctr& ctr, SampleDataSorted& samples, const UUID& key, const Optional<UAcc128T>& value)
    {
        if (value)
        {
            bool result = ctr.remove(key, value.value());
            assert_equals(true, result);
            samples[key].erase(value.value());

            if (samples[key].size() == 0) {
                samples.erase(key);
            }
        }
        else {
            samples.erase(key);
            auto result = ctr.remove(key);
            assert_equals(true, result);
        }
    }

    template <typename Ctr>
    void fillCtr(Ctr& ctr, const SampleDataSorted& samples)
    {
        for (const auto& kv_pair: samples)
        {
            insert_single_batch(ctr, kv_pair.first, kv_pair.second);
        }
    }

    void testRemoveSingleKeyValue()
    {
        samples_ = create_sample_data();
        const auto shuffled_samples = shuffle(samples_);

        auto snp = branch();

        ctr_name_ = create<CtrName>(snp).name();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        fillCtr(ctr, samples_);

        auto t0 = getTimeInMillis();

        size_t cnt = 0;
        for (const auto& kv_pair: shuffled_samples)
        {
            if (cnt % (max_values_ / check_steps_) == 0)
            {
                out() << "Removed " << cnt << " k/v pairs" << std::endl;
            }

            auto key    = kv_pair.first;
            auto value  = kv_pair.second;

            snapshot_.push_back(std::make_pair(key, value));

            remove_pair(ctr, samples_, key, value);

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

        out() << "Values removed in " << FormatTime(getTimeInMillis() - t0) << std::endl;

        check_values(ctr, samples_);

        commit();
    }


    void replayRemoveSingleKeyValue()
    {
        auto snp = branch();

        auto ctr  = find<CtrName>(snp, ctr_name_);

        for (const auto& kv_pair: snapshot_)
        {
            auto key = kv_pair.first;
            auto value  = kv_pair.second;

            if (value) {
                samples_[key].insert(value.value());
            }
            else {
                samples_[key] = std::set<UAcc128T>();
            }
        }

        for (const auto& kv_pair: snapshot_)
        {
            auto key    = kv_pair.first;
            auto value  = kv_pair.second;

            out() << "Removed pair: " << key << " :: " << (value ? value.value().to_bmp().str() : std::string("<none>")) << std::endl;

            remove_pair(ctr, samples_, key, value);

            check_values(ctr, samples_);
        }

        check_values(ctr, samples_);
        commit();
    }

    template <typename Ctr>
    void remove_single_batch(Ctr& ctr, UUID key)
    {
        samples_.erase(key);
        auto result = ctr.remove(key);

        assert_equals(true, result);
    }

    void testRemoveMultipleKeyValues()
    {
        samples_ = create_sample_data();
        auto shuffled_samples = shuffle_keys(samples_);

        auto snp = branch();

        ctr_name_ = create<CtrName>(snp).name();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        fillCtr(ctr, samples_);

        commit();
        snp = branch();
        ctr  = find<CtrName>(snp, ctr_name_);

        auto t0 = getTimeInMillis();

        size_t cnt{};
        for (const auto& key: shuffled_samples)
        {
            snapshot_by_key_.push_back(key);

            remove_single_batch(ctr, key);

            if (cnt % (max_values_ / check_steps_) == 0)
            {
                check_values(ctr, samples_);

                commit();
                snp = branch();
                ctr  = find<CtrName>(snp, ctr_name_);
                snapshot_by_key_.clear();
            }

            cnt++;
        }

        out() << "Values removed in " << FormatTime(getTimeInMillis() - t0) << std::endl;
    }
};



namespace {
auto Suite1 = register_class_suite<EdgeMapValuesRemovalTestSuite>("EdgeMap");
}

}}}
