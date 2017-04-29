
// Copyright 2016 Victor Smirnov
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

#include "packed_string_codec_test.hpp"
#include "packed_int64t_codec_test.hpp"

#ifdef HAVE_BOOST
#include "packed_biginteger_codec_test.hpp"
#endif

namespace memoria {
namespace v1 {

using namespace std;

class PackedCodecsTestSuite: public TestSuite {

public:

    PackedCodecsTestSuite(): TestSuite("Packed.Codecs")
    {
        registerTask(new PackedInt64TCodecTest("Int64T"));
        registerTask(new PackedStringCodecTest("String"));

#ifdef HAVE_BOOST
        registerTask(new PackedBigIntegerCodecTest("BigInteger"));
#endif
    }

};

}}
