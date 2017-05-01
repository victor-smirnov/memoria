
// Copyright 2015 Victor Smirnov
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


#include <memoria/v1/core/types/types.hpp>

#include "vector_test.hpp"

namespace memoria {
namespace v1 {

class VectorTestSuite: public TestSuite {

public:

    VectorTestSuite(): TestSuite("Vector")
    {
        registerTask(new VectorTest<Vector<Int>>("Int.FX"));
        //registerTask(new VectorTest<Vector<VLen<Granularity::Byte>>>("Int.VL.Byte"));
    }
};

MMA1_REGISTER_TEST_SUITE(VectorTestSuite)

}}
