
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REMOVE_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_MAP_VECTORMAP_REMOVE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>



#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;



template <typename Key, typename Value>
class VectorMapRemoveTest: public VectorMapTestBase<VectorMapRemoveTest<Key, Value>, Key, Value> {

    typedef VectorMapRemoveTest<Key, Value>                                     MyType;
    typedef VectorMapTestBase<VectorMapRemoveTest<Key, Value>, Key, Value>      Base;

protected:

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::VMapType                                             VMapType;
    typedef typename Base::TestFn                                               TestFn;


public:

    VectorMapRemoveTest(): Base("Remove")
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(testRandomRemoval, replayRandomRemoval);
    }

    virtual ~VectorMapRemoveTest() throw() {}



    void testRandomRemoval()
    {
        this->testPreFilledMap(&MyType::randomRemovalTest, VMapType::Random, this->size_);
    }

    void replayRandomRemoval()
    {
        this->replay(&MyType::randomRemovalTest, "RandomRemoval");
    }


    void randomRemovalTest(Allocator& allocator, Ctr& map)
    {
        auto& tripples_     = this->tripples_;
        auto& key_          = this->key_;

        if (!this->isReplayMode())
        {
            key_ = tripples_[getRandom(tripples_.size())].id();
        }


        bool removed = map.remove(key_);

        AssertTrue(MA_SRC, removed);

        Int insertion_pos = -1;
        for (UInt idx = 0; idx < tripples_.size(); idx++)
        {
            if (key_ == tripples_[idx].id())
            {
                insertion_pos = idx;
                break;
            }
        }

        this->out()<<key_<<" "<<insertion_pos<<endl;

        AssertGE(MA_SRC, insertion_pos, 0);

        auto tripple = tripples_[insertion_pos];

        tripples_.erase(tripples_.begin() + insertion_pos);

        this->checkMap(map, tripples_, [&]() {
            tripples_.insert(tripples_.begin() + insertion_pos, tripple);
        });
    }
};


}

#endif

