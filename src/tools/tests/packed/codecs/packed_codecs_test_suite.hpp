
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_CODECS_TEST_SUITE_HPP_
#define MEMORIA_TESTS_PACKED_CODECS_TEST_SUITE_HPP_

#include "../../tests_inc.hpp"

#include "packed_string_codec_test.hpp"
#include "packed_biginteger_codec_test.hpp"
#include "packed_int64t_codec_test.hpp"

namespace memoria {

using namespace std;

class PackedCodecsTestSuite: public TestSuite {

public:

    PackedCodecsTestSuite(): TestSuite("Packed.Codecs")
    {
    	registerTask(new PackedInt64TCodecTest("Int64T"));
    	registerTask(new PackedStringCodecTest("String"));
    	registerTask(new PackedBigIntegerCodecTest("BigInteger"));
    }

};

}


#endif

