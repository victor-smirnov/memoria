
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once


#include <memoria/v1/tools/task.hpp>
#include <memoria/v1/tools/tools.hpp>
#include <memoria/v1/tools/benchmarks.hpp>

#include <memoria/v1/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>
#include <vector>

namespace memoria {

using namespace std;



template <typename Profile_, typename Allocator_>
class ProfileBenchmarkTask: public BenchmarkTask {

public:

    typedef Profile_                                Profile;
    typedef Allocator_                              Allocator;


    ProfileBenchmarkTask(StringRef name): BenchmarkTask(name) {}
    virtual ~ProfileBenchmarkTask() throw () {};




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


    virtual void LoadResource(Allocator& allocator, StringRef file_name) const
    {
        String path = getResourcePath(file_name);
        LoadAllocator(allocator, path);
    }

    virtual void StoreResource(Allocator& allocator, StringRef file_name) const
    {
        String path = getResourcePath(file_name);
        StoreAllocator(allocator, path);
    }
};

template<typename T = EmptyType>
class SPBenchmarkTaskT: public ProfileBenchmarkTask<DefaultProfile<>, SmallInMemAllocator> {

    typedef ProfileBenchmarkTask<DefaultProfile<>, SmallInMemAllocator> Base;

public:
    SPBenchmarkTaskT(StringRef name): Base(name) {}
    virtual ~SPBenchmarkTaskT() throw () {};

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

typedef SPBenchmarkTaskT<> SPBenchmarkTask;






}
