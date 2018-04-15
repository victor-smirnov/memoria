
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/tools/tests_inc.hpp>


#include "packed_array_misc_test.hpp"
#include "packed_array_buffer_test.hpp"

#include <memoria/v1/core/tools/exint_codec.hpp>
#include <memoria/v1/core/tools/elias_codec.hpp>
#include <memoria/v1/core/tools/i7_codec.hpp>

namespace memoria {
namespace v1 {

class PackedArrayTestSuite: public TestSuite {

public:

    PackedArrayTestSuite(): TestSuite("Packed.Array")
    {
        registerTask(new PackedArrayMiscTest<PkdVDArrayT<int64_t, 1, UByteExintCodec>>("Misc.VLD.Exint"));
        registerTask(new PackedArrayMiscTest<PkdVDArrayT<int64_t, 1, UInt64EliasCodec>>("Misc.VLD.Elias"));

        registerTask(new PackedArrayInputBufferTest<PkdVDArrayT<int64_t, 4, UByteI7Codec>>("Buffer.4.VLD.I7"));
        registerTask(new PackedArrayInputBufferTest<PkdVDArrayT<int64_t, 4, UInt64EliasCodec>>("Buffer.4.VLD.Elias"));

        registerTask(new PackedArrayInputBufferTest<PkdFSQArrayT<int64_t, 4>>("Buffer.4.FSQ"));
    }

};

}}
