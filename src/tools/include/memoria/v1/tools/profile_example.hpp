
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
#include <memoria/v1/tools/examples.hpp>

#include <memoria/v1/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>
#include <vector>

namespace memoria {
namespace v1 {

using namespace std;

template <typename Profile_, typename Allocator_>
class ProfileExampleTask: public ExampleTask {

public:

    typedef Profile_                                Profile;
    typedef Allocator_                              Allocator;


    ProfileExampleTask(StringRef name): ExampleTask(name) {}
    virtual ~ProfileExampleTask() throw () {};


    virtual void Run(ostream& out)                                          = 0;

    virtual void LoadAllocator(Allocator& allocator, StringRef file_name) const
    {
        unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
        allocator.load(in.get());
    }

    virtual void StoreAllocator(Allocator& allocator, StringRef file_name) const
    {
        unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
        allocator.store(out.get());
    }

};

template <typename T = EmptyType>
class SPExampleTaskT: public ProfileExampleTask<DefaultProfile<>, SmallInMemAllocator> {

    typedef ProfileExampleTask<DefaultProfile<>, SmallInMemAllocator> Base;

public:
    SPExampleTaskT(StringRef name): Base(name) {}
    virtual ~SPExampleTaskT() throw () {};

    virtual void Run(ostream& out)                                          = 0;

    void check(Allocator& allocator, const char* source)
    {
        v1::check<Allocator>(allocator, "Allocator check failed", source);
    }

    void check(Allocator& allocator, const char* message, const char* source)
    {
        v1::check<Allocator>(allocator, message, source);
    }

    template <typename CtrType>
    void checkCtr(CtrType& ctr, const char* message, const char* source)
    {
        v1::checkCtr<CtrType>(ctr, message, source);
    }

    template <typename CtrType>
    void checkCtr(CtrType& ctr, const char* source)
    {
        checkCtr(ctr, "Container check failed", source);
    }
};


typedef SPExampleTaskT<> SPExampleTask;






}}