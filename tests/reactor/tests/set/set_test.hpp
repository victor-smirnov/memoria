
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
#include <functional>

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
        return create_random_string(16 + getRandomG(8));
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
    typename ProfileT = CoreApiProfile,
    typename StoreT   = IMemoryStorePtr<ProfileT>
>
class SetTest: public BTTestBase<Set<DataType>, ProfileT, StoreT>
{
    using MyType = SetTest;

    using Base   = BTTestBase<Set<DataType>, ProfileT, StoreT>;

    using typename Base::CtrApi;

    using typename Base::Store;
    using typename Base::StorePtr;
    using typename Base::CtrID;

    using CxxElementViewType  = DTTViewType<DataType>;

    int64_t size = 1024 * 1024;

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
        MMA_CLASS_TESTS(suite, testOne, testAll);
    }

    void testOne() {
        auto snp = branch();

        CtrID ctr_id = CtrID::make_random();
        auto ctr = create<Set<DataType>>(snp, Set<DataType>{}, ctr_id);
        //ctr->set_new_block_size(1024);

        std::set<CxxValueType> entries_set;
        std::vector<CxxValueType> entries_list;

        int64_t t0 = getTimeInMillis();
        for (int c = 0; c < 100000; c++)
        {
            if (c % 100000 == 0)
            {
                out() << "C=" << c << std::endl;
                this->check("Store structure checking", MMA_SRC);
            }

            auto key = internal_set::ValueTools<CxxValueType>::generate_random();

            entries_set.insert(key);
            entries_list.push_back(key);

            ctr->upsert(key);
        }

        int64_t t1 = getTimeInMillis();
        out() << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

        int c = 0;
        for (auto iter = ctr->seek_entry(ctr->size() - 1); is_valid_chunk(iter); iter = iter->prev(100)) {
            println("Key: {} :: {}", iter->current_key(), iter->entry_offset());
            c++;
        }

        commit();
    }


    void testAll()
    {
        auto snp = branch();

        CtrID ctr_id = CtrID::make_random();
        auto ctr = create<Set<DataType>>(snp, Set<DataType>{}, ctr_id);
        ctr->set_new_block_size(1024);

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

            ctr->upsert(key);
        }
        int64_t t1 = getTimeInMillis();
        out() << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

        out() << "Size: " << ctr->size() << std::endl;

        this->check("Store structure checking", MMA_SRC);

        int64_t t2 = getTimeInMillis();
        for (auto key: entries_list)
        {
            bool kk = ctr->contains(key);
            assert_equals(true, kk);
        }
        int64_t t3 = getTimeInMillis();
        out() << "Queried entries in " << (t3 - t2) << " ms" << std::endl;

        auto scc = ctr->first_entry();
        auto en_ii = entries_set.begin();

        while (is_valid_chunk(scc))
        {
            auto keys = scc->keys();

            for (size_t c = 0; c < keys.size(); c++)
            {
                auto en_key = *en_ii;

                bool equals = internal_set::ValueTools<CxxValueType>::equals(keys[c], en_key);
                assert_equals(true, equals);

                en_ii++;
            }

            scc = scc->next_chunk();
        }


        int64_t t4 = getTimeInMillis();
        size_t cnt = 0;
        auto ctr_size = ctr->size();
        for (auto& key: entries_list)
        {
            if (cnt % 100000 == 0) {
                out() << "K=" << cnt << std::endl;
                this->check("Store structure checking", MMA_SRC);
            }

            ctr->remove(key);

            cnt++;
            ctr_size--;

            assert_equals(ctr_size, ctr->size());
        }

        int64_t t5 = getTimeInMillis();
        println("Removed entries in {} ms", t5 - t4);

        out() << "Final Container Size: " << ctr->size() << std::endl;
        this->check("Store structure checking", MMA_SRC);

        commit();
    }
};


}}
