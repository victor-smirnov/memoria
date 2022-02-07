
// Copyright 2012 Victor Smirnov
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

#include <memoria/profiles/core_api/core_api_profile.hpp>

#include "../prototype/bt/bt_test_base.hpp"

#include <memoria/api/multimap/multimap_api.hpp>
#include <memoria/core/tools/random.hpp>

#include <vector>

namespace memoria {
namespace tests {

template <
    typename KeyDataType,
    typename ValueDataType,
    typename ProfileT = CoreApiProfile,
    typename StoreT   = IMemoryStorePtr<ProfileT>
>
class MultimapTest: public BTTestBase<Multimap<KeyDataType, ValueDataType>, ProfileT, StoreT>
{
    using MyType = MultimapTest;

    using Base   = BTTestBase<Multimap<KeyDataType, ValueDataType>, ProfileT, StoreT>;

    using typename Base::CtrApi;

    using typename Base::Store;
    using typename Base::StorePtr;

    int64_t entries = 1024 * 16;
    int64_t mean_entry_size = 1024;

    using Base::store;
    using Base::snapshot;
    using Base::branch;
    using Base::commit;
    using Base::drop;
    using Base::out;
    using Base::println;
    using Base::getRandom;
    using Base::check;

    using CxxKeyType    = typename DTTCxxValueType<KeyDataType>::Type;
    using CxxValueType  = typename DTTCxxValueType<ValueDataType>::Type;

    using StlMap = std::map<CxxKeyType, CxxValueType>;

    struct Entry {
        CxxKeyType key;
        std::vector<CxxValueType> values;
    };

public:
    MultimapTest(){}

    MMA_STATE_FILEDS(entries)

    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TEST(suite, testBatchOps);
        MMA_CLASS_TEST(suite, testUpsert);
        MMA_CLASS_TEST(suite, testRemove);
    }


    std::vector<Entry> build_entries(size_t entries, size_t mean_value_size)
    {
        std::vector<Entry> data;

        for (size_t c = 0; c < entries; c++)
        {
            std::vector<CxxValueType> values;

            CxxKeyType key = DTTestTools<KeyDataType>::generate_random();

            size_t entry_size = static_cast<size_t>(getRandomG(mean_value_size * 2));

            for (size_t v = 0; v < entry_size; v++) {
                values.push_back(DTTestTools<ValueDataType>::generate_random());
            }

            data.emplace_back(Entry{std::move(key), std::move(values)});
        }

        return data;
    }

    template <typename CtrApiT>
    void populate_container(CtrApiT ctr, const std::vector<Entry>& data)
    {
        int64_t t0 = getTimeInMillis();
        ctr->append_entries([&](auto& seq, auto& keys, auto& values, auto& sizes) {

            size_t batch_start = sizes.entries_;

            size_t batch_size = 8192;
            size_t limit = (batch_start + batch_size <= data.size()) ?
                        batch_size : data.size() - batch_start;

            for (size_t c = 0; c < limit; c++)
            {
                seq.append_run(0, 1);
                keys.append(data[batch_start + c].key);

                auto& entry_data = data[batch_start + c].values;

                if (entry_data.size() > 0)
                {
                    seq.append_run(1, entry_data.size());
                    values.append(entry_data);
                }
            }

            sizes.entries_ += limit;

            return batch_start + limit >= data.size();
        });

        int64_t t1 = getTimeInMillis();
        println("Contianer of size {} was created in {} ms", ctr->size(), t1 - t0);
    }

    template <typename CtrApiT>
    void check_container(CtrApiT ctr, const std::vector<Entry>& data)
    {
        out() << "Checking contianer.... ";

        this->check("Store structure checking", MMA_SRC);

        assert_equals(data.size(), ctr->size());

        auto scanner = ctr->entries_scanner();

        size_t cnt{};
        scanner->for_each([&](auto key, auto values){
            assert_equals(data[cnt].key, key);
            assert_equals(data[cnt].values.size(), values.size());

            size_t vcnt{};
            for (auto& value: values) {
                assert_equals(data[cnt].values[vcnt], value);
                vcnt++;
            }

            cnt++;
        });

        out() << "Done." << std::endl;
    }

    void sort(std::vector<Entry>& data)
    {
        std::sort(data.begin(), data.end(), [](const auto& first, const auto& second){
            return first.key < second.key;
        });
    }


    void testBatchOps()
    {
        auto snp = branch();
        auto ctr = create(snp, Multimap<KeyDataType, ValueDataType>{});
        //ctr->set_new_block_size(2048);

        std::vector<Entry> data = build_entries(entries, mean_entry_size);

        auto data_unsored = data;
        sort(data);

        populate_container(ctr, data);

        check_container(ctr, data);

        for (const auto& entry: data_unsored)
        {
            bool contains = ctr->contains(entry.key);
            assert_equals(true, contains);
        }

        while (ctr->size() > 0)
        {
            size_t ctr_size = static_cast<size_t>(ctr->size());

            size_t from = getRandom(ctr_size);
            size_t del_size = getRandom(1024);
            if (from + del_size > ctr_size) {
                del_size = ctr_size - from;
            }

            size_t to = from + del_size;

            CxxKeyType key_from = data[from].key;

            println("Ctr size: {}, removing [{}, {})", ctr_size, from, to);

            if (to < ctr_size)
            {
                CxxKeyType key_to = data[to].key;
                ctr->remove_all(key_from, key_to);
            }
            else {
                ctr->remove_from(key_from);
            }

            data.erase(data.begin() + from, data.begin() + to);
            check_container(ctr, data);
        }

        commit();
    }

    void testUpsert()
    {
        auto snp = branch();
        auto ctr = create(snp, Multimap<KeyDataType, ValueDataType>{});

        std::vector<Entry> data_unsorted;

        for (size_t c = 0; c < entries; c++)
        {
            if (c % 1024 == 0)
            {
                println("C={}", c);
            }

            CxxKeyType key = DTTestTools<KeyDataType>::generate_random();

            size_t entry_size = static_cast<size_t>(getRandomG(this->mean_entry_size * 2));

            std::vector<CxxValueType> values;
            for (size_t v = 0; v < entry_size; v++) {
                values.push_back(DTTestTools<ValueDataType>::generate_random());
            }

            bool inserted = ctr->upsert(key, values);
            assert_equals(false, inserted, "Upsert");

            bool contains = ctr->contains(key);
            assert_equals(true, contains, "Contains");

            data_unsorted.emplace_back(Entry{std::move(key), std::move(values)});

            if (c % 1024 == 0)
            {
                auto data_sorted = data_unsorted;
                sort(data_sorted);
                check_container(ctr, data_sorted);
            }
        }

        auto data_sorted = data_unsorted;
        sort(data_sorted);
        check_container(ctr, data_sorted);

        commit();
    }

    void testRemove()
    {
        auto snp = branch();
        auto ctr = create(snp, Multimap<KeyDataType, ValueDataType>{});
        //ctr->set_new_block_size(2048);

        size_t max_ctr_size = entries;

        std::vector<Entry> data = build_entries(max_ctr_size, mean_entry_size);
        auto data_unsorted = data;
        sort(data);

        populate_container(ctr, data);
        check_container(ctr, data);

        for (const auto& entry: data_unsorted)
        {
            bool contains = ctr->contains(entry.key);
            assert_equals(true, contains);
        }

        for (size_t c = 0; c < max_ctr_size; c++)
        {
            println("R={}", c);
            CxxKeyType key = data_unsorted[c].key;

            bool removed = ctr->remove(key);
            assert_equals(true, removed, "Remove");
            assert_equals(max_ctr_size - c - 1, ctr->size(), "Size");

            if (c % 1000 == 0) {
                this->check("Store structure checking", MMA_SRC);
            }
        }

        this->check("Store structure checking", MMA_SRC);

        commit();
    }
};


}}
