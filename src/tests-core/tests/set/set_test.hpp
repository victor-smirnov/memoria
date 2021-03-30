
// Copyright 2020 Victor Smirnov
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

#include "../prototype/bt/bt_test_base.hpp"

#include <memoria/tests/assertions.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/core/tools/random.hpp>

#include <vector>

namespace memoria {
namespace tests {

namespace internal_set {

template <typename CxxValueType> struct ValueTools;

template <>
struct ValueTools<UUID> {
    static UUID generate_random() noexcept {
        uint64_t hi_val = static_cast<uint64_t>(getBIRandomG());
        uint64_t lo_val = static_cast<uint64_t>(getBIRandomG());
        return UUID(hi_val, lo_val);
    }

    static bool equals(const UUID& one, const UUID& two) noexcept {
        return one == two;
    }
};

template <>
struct ValueTools<U8String> {
    static U8String generate_random() noexcept {
        return create_random_string(16);
    }

    template <typename One, typename Two>
    static bool equals(const One& one, const Two& two) noexcept {
        return one == two.view();
    }
};

}


template <
    typename DataType,
    typename CxxValueType,
    typename ProfileT = CoreApiProfile<>,
    typename StoreT   = IMemoryStorePtr<ProfileT>
>
class SetTest: public BTTestBase<Set<DataType>, ProfileT, StoreT>
{
    using MyType = SetTest;

    using Base   = BTTestBase<Set<DataType>, ProfileT, StoreT>;

    using typename Base::CtrApi;

    using typename Base::Store;
    using typename Base::StorePtr;

    using CxxElementViewType  = DTTViewType<DataType>;

    int64_t size = 1024 * 1024 * 2;

    using Base::store;
    using Base::snapshot;
    using Base::branch;
    using Base::commit;
    using Base::drop;
    using Base::println;
    using Base::out;
    using Base::getRandom;

public:
    SetTest()
    {
    }

    MMA_STATE_FILEDS(size)

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TEST(suite, testAll);
    }




    void testAll()
    {
        auto snp = branch();

        UUID ctr_id = UUID::parse("30b33f35-96a4-4502-82f4-4bbe50e59c51");
        auto ctr = create<Set<DataType>>(snp, Set<DataType>{}, ctr_id).get_or_throw();
        ctr->set_new_block_size(1024).get_or_throw();

        std::set<CxxValueType> entries_set;
        std::vector<CxxValueType> entries_list;

        int64_t t0 = getTimeInMillis();
        for (int c = 0; c < size; c++)
        {
            if (c % 100000 == 0)
            {
                out() << "C=" << c << std::endl;
                this->check("Store structure checking", MMA_SRC);
            }

            auto key = internal_set::ValueTools<CxxValueType>::generate_random();

            entries_set.insert(key);
            entries_list.push_back(key);

            ctr->insert(key).get_or_throw();
        }
        int64_t t1 = getTimeInMillis();
        out() << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

        out() << "Size: " << ctr->size().get_or_throw() << std::endl;

        this->check("Store structure checking", MMA_SRC);

        int64_t t2 = getTimeInMillis();
        for (auto key: entries_list)
        {
            bool kk = ctr->contains(key).get_or_throw();
            assert_equals(true, kk);
        }
        int64_t t3 = getTimeInMillis();
        out() << "Queried entries in " << (t3 - t2) << " ms" << std::endl;

        auto scc = ctr->scanner();
        auto en_ii = entries_set.begin();

        while (!scc.is_end())
        {
            for (auto key: scc.keys())
            {
                auto en_key = *en_ii;

                bool equals = internal_set::ValueTools<CxxValueType>::equals(key, en_key);
                assert_equals(true, equals);

                en_ii++;
            }

            scc.next_leaf().get_or_throw();
        }


        int64_t t4 = getTimeInMillis();
        size_t cnt = 0;
        auto ctr_size = ctr->size().get_or_throw();
        for (auto& key: entries_list)
        {
            if (cnt % 100000 == 0) {
                out() << "K=" << cnt << std::endl;
                this->check("Store structure checking", MMA_SRC);
            }

            ctr->remove(key).get_or_throw();

            cnt++;
            ctr_size--;

            assert_equals(ctr_size, ctr->size().get_or_throw());
        }
        int64_t t5 = getTimeInMillis();
        println("Removed entries in {} ms", t5 - t4);

        out() << "Final Container Size: " << ctr->size().get_or_throw() << std::endl;
        this->check("Store structure checking", MMA_SRC);

        commit();
    }


};


}}
