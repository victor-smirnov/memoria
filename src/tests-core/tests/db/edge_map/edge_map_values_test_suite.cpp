
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

class EdgeMapValuesTestSuite: public BTTestBase<EdgeMap, InMemAllocator<>, DefaultProfile<>> {
public:
    using Base   = BTTestBase<EdgeMap, InMemAllocator<>, DefaultProfile<>>;
    using MyType = EdgeMapValuesTestSuite;

    using typename Base::Ctr;
    using typename Base::CtrName;

    using Base::branch;
    using Base::commit;
    using Base::allocator;
    using Base::check;
    using Base::out;

    std::vector<UUID> keys_;
    UUID current_key_;

    MMA1_INDIRECT_STATE_FILEDS(keys_, current_key_)

    EdgeMapValuesTestSuite()
    {
    }

    static void init_suite(TestSuite& suite)
    {
        //MMA1_CLASS_TEST_WITH_REPLAY(suite, testCreateSimple, replayCreateSimple);
        MMA1_CLASS_TESTS(suite, testCreateSimpleValues);
    }

    void testCreateSimpleValues() {

    }

//    void testCreateSimple()
//    {
//        auto snp = branch();

//        auto ctr_name = create<CtrName>(snp).name();
//        auto ctr 	  = find<CtrName>(snp, ctr_name);

//        std::set<UUID> keys;

//        for (int c = 0; c < 1000000; c++)
//        {
//            UUID key = current_key_ = UUID::make_random();
//            keys.insert(key);

//            auto ii1 = ctr.find(key);
//            assert_equals(false, ii1.is_found(key));

//            auto ii2 = ctr.find_or_create(key);
//            assert_equals(true, ii2.is_found(key));

//            keys_.push_back(key);
//        }

//        commit();
//    }

//    template <typename Ctr>
//    void doDimpleTest(Ctr& ctr, UUID key)
//    {
//        auto ii1 = ctr.find(key);
//        assert_equals(false, ii1.is_found(key));

//        auto ii2 = ctr.find_or_create(key);
//        assert_equals(true, ii2.is_found(key));
//    }

//    void replayCreateSimple()
//    {
//        out() << "Replaying createTest: " << keys_.size() << std::endl;

//        auto snp = branch();

//        auto ctr_name = create<CtrName>(snp).name();
//        auto ctr 	  = find<CtrName>(snp, ctr_name);

//        for (size_t c = 0; c < keys_.size(); c++)
//        {
//            UUID key = keys_[c];
//            doDimpleTest(ctr, key);
//        }

//        DebugCounter = 1;
//        doDimpleTest(ctr, current_key_);

//        commit();
//    }
};



namespace {

auto Suite1 = register_class_suite<EdgeMapValuesTestSuite>("EdgeMap");

}

}}}
