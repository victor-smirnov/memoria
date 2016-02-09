
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_PROFILE_TESTS_HPP
#define _MEMORIA_TOOLS_PROFILE_TESTS_HPP


#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>

namespace memoria {

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
            ::memoria::check<Allocator>(allocator, "Allocator check failed", source);
        }

        check_count++;
    }

    void forceCheck(const std::shared_ptr<Allocator>& allocator, const char* source)
    {
        ::memoria::check<Allocator>(allocator, "Allocator check failed", source);
    }

    void check(const std::shared_ptr<Allocator>& allocator, const char* message, const char* source)
    {
        Int step_count = getcheckStep();

        if (check_count % step_count == 0)
        {
            ::memoria::check<Allocator>(allocator, message, source);
        }
        check_count++;
    }

    template <typename CtrType>
    void checkCtr(CtrType& ctr, const char* message, const char* source)
    {
        Int step_count = getcheckStep();

        if (step_count > 0 && (check_count % step_count == 0))
        {
            ::memoria::checkCtr<CtrType>(ctr, message, source);
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




}
#endif
