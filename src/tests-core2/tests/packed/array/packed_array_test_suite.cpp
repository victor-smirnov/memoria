
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include "packed_array_buffer_test.hpp"
#include "packed_array_misc_test.hpp"


#include <memoria/v1/core/tools/exint_codec.hpp>
#include <memoria/v1/core/tools/elias_codec.hpp>
#include <memoria/v1/core/tools/i7_codec.hpp>


namespace memoria {
namespace v1 {
namespace tests {

namespace {

using Suite1 = PackedArrayMiscTest<PkdVDArrayT<int64_t, 1, UByteExintCodec>>;
MMA1_CLASS_SUITE(Suite1, "Array.Misc.VLD.Exint");

using Suite2 = PackedArrayMiscTest<PkdVDArrayT<int64_t, 1, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite2, "Array.Misc.VLD.Elias");

using Suite3 = PackedArrayInputBufferTest<PkdVDArrayT<int64_t, 4, UByteI7Codec>>;
MMA1_CLASS_SUITE(Suite3, "Array.Buffer.4.VLD.I7");

using Suite4 = PackedArrayInputBufferTest<PkdVDArrayT<int64_t, 4, UInt64EliasCodec>>;
MMA1_CLASS_SUITE(Suite4, "Array.Buffer.4.VLD.Elias");

using Suite5 = PackedArrayInputBufferTest<PkdFSQArrayT<int64_t, 4>>;
MMA1_CLASS_SUITE(Suite5, "Array.Buffer.4.FSQ");

}


}}}
