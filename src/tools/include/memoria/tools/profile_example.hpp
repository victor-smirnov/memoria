
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_PROFILE_EXAMPLE_HPP
#define _MEMORIA_TOOLS_PROFILE_EXAMPLE_HPP


#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/tools/examples.hpp>

#include <memoria/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>
#include <vector>

namespace memoria {

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
        ::memoria::check<Allocator>(allocator, "Allocator check failed", source);
    }

    void check(Allocator& allocator, const char* message, const char* source)
    {
        ::memoria::check<Allocator>(allocator, message, source);
    }

    template <typename CtrType>
    void checkCtr(CtrType& ctr, const char* message, const char* source)
    {
        ::memoria::checkCtr<CtrType>(ctr, message, source);
    }

    template <typename CtrType>
    void checkCtr(CtrType& ctr, const char* source)
    {
        checkCtr(ctr, "Container check failed", source);
    }
};


typedef SPExampleTaskT<> SPExampleTask;






}
#endif
