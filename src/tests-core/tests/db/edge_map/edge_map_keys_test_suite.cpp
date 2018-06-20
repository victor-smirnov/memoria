
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

#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>

#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>

namespace memoria {
namespace v1 {
namespace tests {

class EdgeMapKeysTestSuite: public BTTestBase<EdgeMap, InMemAllocator<>, DefaultProfile<>> {
public:
    using Base   = BTTestBase<EdgeMap, InMemAllocator<>, DefaultProfile<>>;
    using MyType = EdgeMapKeysTestSuite;

    using typename Base::Ctr;
    using typename Base::CtrName;

    using Base::branch;
    using Base::commit;
    using Base::snapshot;
    using Base::check;
    using Base::out;
    using Base::getRandom;

    std::set<UUID> keys_;
    std::vector<UUID> keys_snapshot_;
    std::vector<UUID> keys_shuffled_;

    UUID ctr_name_;

    size_t max_keys_{3000000};
    size_t commit_rate_{10000};

    MMA1_STATE_FILEDS(commit_rate_, ctr_name_)
    MMA1_INDIRECT_STATE_FILEDS(keys_, keys_snapshot_, keys_shuffled_)

    EdgeMapKeysTestSuite(){}

    static void init_suite(TestSuite& suite)
    {
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testInsertKey, replayInsertKey);
        MMA1_CLASS_TEST_WITH_REPLAY(suite, testRemoveKey, replayRemoveKey);
    }

    virtual void post_configure(TestCoverage coverage)
    {
        max_keys_ = select_for_coverage<size_t>(
            coverage,
            1000,
            10000,
            100000,
            1000000,
            10000000,
            100000000
        );
    }


    void testInsertKey()
    {
        auto snp = branch();

        ctr_name_ = create<CtrName>(snp).name();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        auto t0 = getTimeInMillis();

        for (size_t c = 0; c < max_keys_; c++)
        {
            if (c % (max_keys_ / 100) == 0) {
                out() << "Inserted " << c << " keys" << std::endl;
            }

            UUID key = UUID::make_random();

            keys_snapshot_.push_back(key);
            keys_.insert(key);

            doSimpleTest(ctr, key);

            if (c % commit_rate_ == 0)
            {
                commit();

                snp = branch();
                ctr = find<CtrName>(snp, ctr_name_);

                keys_snapshot_.clear();
            }
        }

        out() << "Keys inserted in " << FormatTime(getTimeInMillis() - t0) << std::endl;

        check_keys(ctr, keys_);

        commit();
    }

    void replayInsertKey()
    {
        out() << "Replaying createTest. Snapshot size " << keys_snapshot_.size() << std::endl;

        auto snp = branch();

        auto ctr = find<CtrName>(snp, ctr_name_);

        for (size_t c = 0; c < keys_snapshot_.size(); c++)
        {
            UUID key = keys_snapshot_[c];
            doSimpleTest(ctr, key);
        }

        commit();
    }

    void testRemoveKey()
    {
        auto snp = branch();

        ctr_name_ = create<CtrName>(snp).name();
        auto ctr  = find<CtrName>(snp, ctr_name_);

        auto tt = getTimeInMillis();

        for (size_t cnt = 0; cnt < max_keys_; cnt++)
        {
            UUID key = UUID::make_random();
            keys_.insert(key);
            keys_shuffled_.push_back(key);
        }

        out() << "Data populated in " << FormatTime(getTimeInMillis() - tt) << std::endl;

        using SetIterator = std::set<UUID>::iterator;

        InputIteratorProvider<UUID, SetIterator, SetIterator, CtrIOBuffer> keys_provider(keys_.begin(), keys_.end());
        edge_map::SingleStreamProducerAdapter<CtrIOBuffer, 2> adaptor(keys_provider, 0);

        auto t0 = getTimeInMillis();

        auto ii = ctr.begin();
        ii.insert_subseq(adaptor);

        out() << "Keys created in " << FormatTime(getTimeInMillis() - t0) << std::endl;

        check_keys(ctr, keys_);

        //commit();
        //snp = branch();
        //ctr = find<CtrName>(snp, ctr_name_);

        auto t1 = getTimeInMillis();

        commit_rate_ = max_keys_;

        for (size_t cnt = 0; cnt < max_keys_; cnt++)
        {
            if (cnt % (max_keys_ / 100) == 0)
            {
                out () << "Keys removed: " << cnt << std::endl;
            }

            auto key = keys_shuffled_[cnt];

            keys_snapshot_.push_back(key);
            keys_.erase(key);

            ctr.remove(key);

            if (cnt % (max_keys_ / 10) == 0)
            {
                check_keys(ctr, keys_);
            }

            if (false && cnt % commit_rate_ == 0)
            {
                commit();

                snp = branch();
                ctr = find<CtrName>(snp, ctr_name_);

                keys_snapshot_.clear();
            }
        }

        out() << "Keys removed in " << FormatTime(getTimeInMillis() - t1) << std::endl;

        commit();
    }

    void replayRemoveKey()
    {
        out() << "Replaying removeTest. Snapshot size " << keys_snapshot_.size() << std::endl;

        for (auto& kk: keys_snapshot_) {
            keys_.insert(kk);
        }

        auto snp = branch();

        auto ctr = find<CtrName>(snp, ctr_name_);

        for (size_t c = 0; c < keys_snapshot_.size(); c++)
        {
            UUID key = keys_snapshot_[c];
            ctr.remove(key);
        }

        check_keys(ctr, keys_);

        commit();
    }


    template <typename Ctr>
    void check_keys(Ctr& ctr, const std::set<UUID>& keys)
    {
        assert_equals(keys.size(), (size_t)ctr.size());

        auto ki = keys.begin();
        for (auto keys_ii = ctr.keys(); keys_ii.has_keys(); ki++)
        {
            auto k1 = keys_ii.key();
            auto k2 = *ki;

            assert_equals(k2, k1);

            keys_ii.next_key();
        }
    }

    template <typename Ctr>
    void doSimpleTest(Ctr& ctr, UUID key)
    {
        auto ii1 = ctr.find(key);
        assert_equals(false, ii1.is_found(key));

        auto ii2 = ctr.find_or_create(key);
        assert_equals(true, ii2.is_found(key));
    }
};



namespace {

auto Suite1 = register_class_suite<EdgeMapKeysTestSuite>("EdgeMap");

}

}}}
