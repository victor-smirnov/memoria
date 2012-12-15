// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_REMOVE_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_REMOVE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "map_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

class MapRemoveTest: public MapTestBase {

    typedef MapRemoveTest                                                       MyType;


    public:

    MapRemoveTest(): MapTestBase("Remove")
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(runRemoveTest, replayRemoveTest);
    }

    virtual ~MapRemoveTest() throw () {}

    void runRemoveTest(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr map(&allocator);

        map.setNewPageSize(8192);

        ctr_name_ = map.name();

        map.setBranchingFactor(btree_branching_);

        try {
            for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
            {
                auto iter = map[pairs[vector_idx_].key_];
                iter.setData(pairs[vector_idx_].value_);
            }

            allocator.commit();

            check(allocator, MEMORIA_SOURCE);

            for (auto iter = map.begin(); iter != map.endm(); iter++)
            {
                pairs_sorted.push_back(Pair(iter.key(), iter.value()));
            }

            for (vector_idx_ = 0; vector_idx_ < size_; vector_idx_++)
            {
                bool result = map.remove(pairs[vector_idx_].key_);

                MEMORIA_TEST_THROW_IF(result, !=, true);

                check(allocator, MEMORIA_SOURCE);

                BigInt size = size_ - vector_idx_ - 1;

                MEMORIA_TEST_THROW_IF(size, !=, map.getSize());

                PairVector pairs_sorted_tmp = pairs_sorted;

                for (UInt x = 0; x < pairs_sorted_tmp.size(); x++)
                {
                    if (pairs_sorted_tmp[x].key_ == pairs[vector_idx_].key_)
                    {
                        pairs_sorted_tmp.erase(pairs_sorted_tmp.begin() + x);
                    }
                }

                checkContainerData(map, pairs_sorted_tmp);

                allocator.commit();

                check(allocator, MEMORIA_SOURCE);

                pairs_sorted = pairs_sorted_tmp;
            }
        }
        catch (...)
        {
            StorePairs(pairs, pairs_sorted);
            dump_name_ = Store(allocator);
            throw;
        }
    }

    void replayRemoveTest(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);
        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        LoadAllocator(allocator, dump_name_);

        LoadVector(pairs, pairs_data_file_);
        LoadVector(pairs_sorted, pairs_sorted_data_file_);

        Ctr map(&allocator, ctr_name_);

        bool result = map.remove(pairs[vector_idx_].key_);

        MEMORIA_TEST_THROW_IF(result, !=, true);

        check(allocator, MEMORIA_SOURCE);

        BigInt size = size_ - vector_idx_ - 1;

        MEMORIA_TEST_THROW_IF(size, !=, map.getSize());

        for (UInt x = 0; x < pairs_sorted.size(); x++)
        {
            if (pairs_sorted[x].key_ == pairs[vector_idx_].key_)
            {
                pairs_sorted.erase(pairs_sorted.begin() + x);
            }
        }

        checkContainerData(map, pairs_sorted);

        allocator.commit();

        check(allocator, MEMORIA_SOURCE);
    }

};

}

#endif