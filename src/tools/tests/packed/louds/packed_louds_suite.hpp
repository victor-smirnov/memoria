
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../tests_inc.hpp"


#include "packed_louds_create_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class PackedLoudsTestSuite: public TestSuite {
public:

    PackedLoudsTestSuite(): TestSuite("Packed.Louds")
    {
        registerTask(new PackedLoudsCreateTest());
    }
};

}}