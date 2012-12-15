// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_CREATE_TEST_HPP_
#define MEMORIA_TESTS_MAP_MAP_CREATE_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "map_test_base.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

class MapCreateTest: public MapTestBase {

    typedef MapCreateTest                                                       MyType;

public:

    MapCreateTest(): MapTestBase("Create")
    {
        MEMORIA_ADD_TEST_WITH_REPLAY(runCreateTest, replayCreateTest);
    }

    virtual ~MapCreateTest() throw () {}


    void runCreateTest(ostream& out)
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

                checkIterator(out, iter, MEMORIA_SOURCE);

                check(allocator, MEMORIA_SOURCE);

                PairVector tmp = pairs_sorted;

                appendToSortedVector(tmp, pairs[vector_idx_]);

                checkContainerData(map, tmp);

                allocator.commit();

                check(allocator, MEMORIA_SOURCE);

                pairs_sorted = tmp;
            }
        }
        catch (...) {
            StorePairs(pairs, pairs_sorted);
            dump_name_ = Store(allocator);
            throw;
        }
    }

    void replayCreateTest(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);
        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        LoadAllocator(allocator, dump_name_);

        LoadVector(pairs, pairs_data_file_);
        LoadVector(pairs_sorted, pairs_sorted_data_file_);

        Ctr map(&allocator, ctr_name_);

        auto iter = map[pairs[vector_idx_].key_];
        iter.setData(pairs[vector_idx_].value_);

        checkIterator(out, iter, MEMORIA_SOURCE);

        check(allocator, MEMORIA_SOURCE);

        appendToSortedVector(pairs_sorted, pairs[vector_idx_]);

        checkContainerData(map, pairs_sorted);

        allocator.commit();

        check(allocator, MEMORIA_SOURCE);
    }
};

}

#endif
