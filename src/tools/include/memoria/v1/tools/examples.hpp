
// Copyright 2012 Victor Smirnov
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


#include <memoria/v1/tools/task.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <map>
#include <memory>
#include <fstream>

namespace memoria {
namespace v1 {

using namespace std;


class ExampleTask: public Task {

public:
    Int     size_;
    Int     btree_branching_;
    bool    btree_random_airity_;



public:
    ExampleTask(String name): Task(name), size_(1024), btree_branching_(0), btree_random_airity_(false)
    {
        Add("size", size_);
        Add("btree_branching", btree_branching_);
        Add("btree_random_airity", btree_random_airity_);

        own_folder = true;
    }

    virtual ~ExampleTask() throw () {}

    virtual void Run(ostream& out)                                          = 0;

public:

    String getFileName(StringRef name) const;
};

}}