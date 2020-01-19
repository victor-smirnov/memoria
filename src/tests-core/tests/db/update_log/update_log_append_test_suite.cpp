
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
#include "update_log_test_base.hpp"

#include <memoria/api/db/update_log/update_log_api.hpp>

#include <set>
#include <map>
#include <vector>

namespace memoria {
namespace tests {

class UpdateLogAppendTestSuite: public UpdateLogTestBase {
public:
    using Base   = UpdateLogTestBase;
    using MyType = UpdateLogAppendTestSuite;


    UpdateLogAppendTestSuite(){}

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testAppendCommands, replayAppendCommands);
    }

//    virtual void post_configure(TestCoverage coverage)
//    {
//        max_values_ = select_for_coverage<size_t>(
//            coverage,
//            1000,
//            10000,
//            100000,
//            1000000,
//            10000000,
//            100000000
//        );

//        max_keys_fraction_ = select_for_coverage<size_t>(
//            coverage,
//            100,
//            100,
//            1000,
//            10000,
//            10000,
//            100000
//        );
//    }



    template <typename Ctr>
    void insert_pair(Ctr& ctr, SampleDataSorted& samples, UUID key, const std::vector<uint8_t>& data)
    {
        //ctr.append_commands(key, data.begin(), data.end());

        auto iov = ctr.create_iovector();

        iov->symbol_sequence().append(1, 1);
        iov->symbol_sequence().append(2, data.size());

        auto& ctrid_ss = io::substream_cast<io::IOColumnwiseFixedSizeArraySubstream<UUID>>(iov->substream(1));
        auto& log_data_ss = io::substream_cast<io::IORowwiseFixedSizeArraySubstream<uint8_t>>(iov->substream(2));

        ctrid_ss.append(0, key);
        auto* ptr = log_data_ss.reserve(data.size());
        MemCpyBuffer(data.data(), ptr, data.size());

        auto& s_data = samples[key];
        s_data.insert(s_data.end(), data.begin(), data.end());
    }

    void testAppendCommands()
    {
        auto sample_data = create_sample_data();
        const auto shuffled_samples = shuffle(sample_data);

        auto snp = branch();

        ctr_name_ = create<CtrName>(snp).name();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        ctr.create_snapshot(snp.uuid());

        auto t0 = getTimeInMillis();

        SampleDataSorted samples_tmp;

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

            insert_pair(ctr, samples_tmp, key, value);

            if (cnt % (max_values_ / check_steps_) == 0)
            {
                check_values(ctr, samples_tmp, cnt);

                samples_ = samples_tmp;

                commit();
                snp = branch();
                ctr  = find<CtrName>(snp, ctr_name_);
                snapshot_.clear();
            }

            cnt++;
        }

        out() << "Values inserted in " << FormatTime(getTimeInMillis() - t0) << std::endl;

        check_values(ctr, samples_tmp);

        commit();
    }


    void replayAppendCommands()
    {
        auto snp = branch();

        auto ctr  = find<CtrName>(snp, ctr_name_);

        for (const auto& kv_pair: snapshot_)
        {
            auto key    = kv_pair.first;
            auto value  = kv_pair.second;

            insert_pair(ctr, samples_, key, value);

            check_values(ctr, samples_);
        }

        check_values(ctr, samples_);
        commit();
    }
};



namespace {
auto Suite1 = register_class_suite<UpdateLogAppendTestSuite>("UpdateLog");
}

}}
