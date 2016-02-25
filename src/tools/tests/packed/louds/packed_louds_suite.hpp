
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_LOUDS_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_LOUDS_SUITE_HPP_

#include "../../tests_inc.hpp"


#include "packed_louds_create_test.hpp"

namespace memoria {

using namespace std;

class PackedLoudsTestSuite: public TestSuite {
public:

    PackedLoudsTestSuite(): TestSuite("Packed.Louds")
    {
        registerTask(new PackedLoudsCreateTest());
    }
};

}


#endif
