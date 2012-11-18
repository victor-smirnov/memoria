
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_EXAMPLES_HPP
#define _MEMORIA_TOOLS_EXAMPLES_HPP


#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>

#include <map>
#include <memory>
#include <fstream>

namespace memoria {

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








}
#endif
