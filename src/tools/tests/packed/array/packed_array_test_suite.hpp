
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../../tests_inc.hpp"


#include "packed_array_misc_test.hpp"
#include "packed_array_buffer_test.hpp"

namespace memoria {
namespace v1 {

using namespace std;

class PackedArrayTestSuite: public TestSuite {

public:

    PackedArrayTestSuite(): TestSuite("Packed.Array")
    {
        registerTask(new PackedArrayMiscTest<PkdVDArrayT<BigInt, 1, UByteExintCodec>>("Misc.VLD.Exint"));
        registerTask(new PackedArrayMiscTest<PkdVDArrayT<BigInt, 1, UBigIntEliasCodec>>("Misc.VLD.Elias"));

        registerTask(new PackedArrayInputBufferTest<PkdVDArrayT<BigInt, 4, UByteI7Codec>>("Buffer.4.VLD.I7"));
        registerTask(new PackedArrayInputBufferTest<PkdVDArrayT<BigInt, 4, UBigIntEliasCodec>>("Buffer.4.VLD.Elias"));

        registerTask(new PackedArrayInputBufferTest<PkdFSQArrayT<BigInt, 4>>("Buffer.4.FSQ"));
    }

};

}}