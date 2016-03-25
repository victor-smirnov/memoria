
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


#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/tools.hpp>
#include <memoria/v1/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>

namespace memoria {
namespace v1 {

using namespace std;





template <typename Profile_, typename Allocator_>
class ProfileTestTask: public TestTask {
    typedef ProfileTestTask<Profile_, Allocator_>                               MyType;
public:

    typedef Profile_                                                            Profile;
    typedef Allocator_                                                          Allocator;

    Int     check_count = 0;


    ProfileTestTask(StringRef name): TestTask(name)
    {
        MEMORIA_ADD_TEST_PARAM(check_count);
    }

    virtual ~ProfileTestTask() throw () {};

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

    virtual void StoreResource(Allocator& allocator, StringRef file_name, Int mark = 0) const {
        StoreAllocator(allocator, getResourcePath((SBuf()<<file_name<<mark<<".dump").str()));
    }

    virtual void LoadResource(Allocator& allocator, StringRef file_name, Int mark = 0) const {
        LoadAllocator(allocator, getResourcePath((SBuf()<<file_name<<mark<<".dump").str()));
    }


    virtual String Store(Allocator& allocator) const
    {
        String file_name = getAllocatorFileName(".valid");
        StoreAllocator(allocator, file_name);

        String file_name_invalid = getAllocatorFileName(".invalid");
        allocator.commit();
        StoreAllocator(allocator, file_name_invalid);

        return file_name;
    }



    virtual String getAllocatorFileName(StringRef infix = "") const
    {
        return getResourcePath("Allocator"+infix+".dump");
    }
};

template <typename T = EmptyType>
class SPTestTaskT: public ProfileTestTask<DefaultProfile<>, PersistentInMemAllocator<DefaultProfile<>>> {

    using Base = ProfileTestTask<DefaultProfile<>, PersistentInMemAllocator<DefaultProfile<>>>;

    using Base::Allocator;

public:
    SPTestTaskT(StringRef name): Base(name) {}
    virtual ~SPTestTaskT() throw () {};

    void check(const std::shared_ptr<Allocator>& allocator, const char* source)
    {
        Int step_count = getcheckStep();

        if (step_count > 0 && (check_count % step_count == 0))
        {
            v1::check<Allocator>(allocator, "Allocator check failed", source);
        }

        check_count++;
    }

    void forceCheck(const std::shared_ptr<Allocator>& allocator, const char* source)
    {
        v1::check<Allocator>(allocator, "Allocator check failed", source);
    }

    void check(const std::shared_ptr<Allocator>& allocator, const char* message, const char* source)
    {
        Int step_count = getcheckStep();

        if (check_count % step_count == 0)
        {
            v1::check<Allocator>(allocator, message, source);
        }
        check_count++;
    }

    template <typename CtrType>
    void checkCtr(CtrType& ctr, const char* message, const char* source)
    {
        Int step_count = getcheckStep();

        if (step_count > 0 && (check_count % step_count == 0))
        {
            v1::checkCtr<CtrType>(ctr, message, source);
        }

        check_count++;
    }

    template <typename CtrType>
    void checkCtr(CtrType& ctr, const char* source)
    {
        checkCtr(ctr, "Container check failed", source);
    }
};

typedef SPTestTaskT<> SPTestTask;




}}