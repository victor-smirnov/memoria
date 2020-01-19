
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


#include <memoria/tools/tests_inc.hpp>

#include "ltree_iter_test.hpp"
#include "ltree_create_test.hpp"
#include "ltree_remove_test.hpp"

#include <vector>

namespace memoria {

class LabeledTreeTestSuite: public TestSuite {

public:

    LabeledTreeTestSuite(): TestSuite("LabeledTree")
    {
        registerTask(new LabeledTreeIterTest());
        registerTask(new LabeledTreeCreateTest());
        registerTask(new LabeledTreeRemoveTest());
    }

};

}
