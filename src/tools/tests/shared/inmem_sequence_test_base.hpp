
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SHARED_INMEM_SEQUENCE_TEST_BASE_HPP_
#define MEMORIA_TESTS_SHARED_INMEM_SEQUENCE_TEST_BASE_HPP_

#include <memoria/memoria.hpp>
#include <memoria/tools/tests.hpp>

#include "abstract_ralist_test_base.hpp"

#include <vector>

namespace memoria {

using namespace memoria::vapi;
using namespace std;

template <
    typename ContainerTypeName,
    typename MemBuffer
>
class SequenceCreateTestBase: public AbstractRandomAccessListTestBase <
                                        SmallProfile<>,
                                        typename SmallInMemAllocator::WalkableAllocator,
                                        ContainerTypeName,
                                        MemBuffer
                              >
{
    typedef SequenceCreateTestBase<ContainerTypeName, MemBuffer>                MyType;
    typedef MyType                                                              ParamType;


    typedef AbstractRandomAccessListTestBase <
            SmallProfile<>,
            typename SmallInMemAllocator::WalkableAllocator,
            ContainerTypeName,
            MemBuffer
    >                                                                           Base;

protected:
    typedef typename Base::Ctr                                                  Ctr;
    typedef typename Base::Iterator                                             Iterator;
    typedef typename Base::ID                                                   ID;

    typedef typename Base::TestFn                                               TestFn;

    typedef SmallInMemAllocator                                                 Allocator;

    Int cnt_ = 0;

public:
    SequenceCreateTestBase(StringRef name):
        Base(name)
    {}

    virtual String getAllocatorFileName(StringRef infix = "") const
    {
        return this->getResourcePath("Allocator"+infix+".dump");
    }

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
        StoreAllocator(allocator, this->getResourcePath((SBuf()<<file_name<<mark<<".dump").str()));
    }

    virtual void LoadResource(Allocator& allocator, StringRef file_name, Int mark = 0) const {
        LoadAllocator(allocator, this->getResourcePath((SBuf()<<file_name<<mark<<".dump").str()));
    }


    virtual String Store(Allocator& allocator) const
    {
        String file_name = this->getAllocatorFileName(".valid");
        StoreAllocator(allocator, file_name);

        String file_name_invalid = this->getAllocatorFileName(".invalid");
        allocator.commit();
        StoreAllocator(allocator, file_name_invalid);

        return file_name;
    }

    virtual void compareBuffers(const MemBuffer& src, const MemBuffer& tgt, const char* source)
    {
        AssertEQ(source, src.size(), tgt.size(), SBuf()<<"buffer sizes are not equal");

        for (size_t c = 0; c < src.size(); c++)
        {
            typename MemBuffer::value_type v1 = src[c];
            typename MemBuffer::value_type v2 = tgt[c];

            AssertEQ(source, v1, v2, [=](){return SBuf()<<"c="<<c;});
        }
    }

    ostream& out() {
        return Base::out();
    }


    void checkIterator(Iterator& iter, const char* source)
    {
        Base::checkIterator(iter, source);
    }


    virtual void testInsert(TestFn test_fn)
    {
        Allocator allocator;
        DefaultLogHandlerImpl logHandler(out());
        allocator.getLogger()->setHandler(&logHandler);
        allocator.getLogger()->level() = Logger::ERROR;

        Ctr ctr(&allocator);
        this->ctr_name_ = ctr.name();

        allocator.commit();

        try {
            while (ctr.size() < this->size_)
            {
                test_fn(this, allocator, ctr);

                out()<<"Size: "<<ctr.size()<<endl;

                this->checkAllocator(allocator, "Insert: Container Check Failed", MA_SRC);

                allocator.commit();
            }
        }
        catch (...) {
            this->dump_name_ = Store(allocator);
            throw;
        }
    }


    virtual void testRemove(TestFn test_fn)
    {
        Allocator allocator;
        DefaultLogHandlerImpl logHandler(out());
        allocator.getLogger()->setHandler(&logHandler);

        Ctr ctr(&allocator);
        this->ctr_name_ = ctr.name();

        allocator.commit();

        try {

            this->fillRandom(ctr, Base::size_);

            allocator.commit();

            while (ctr.size() > 0)
            {
                test_fn(this, allocator, ctr);

                out()<<"Size: "<<ctr.size()<<endl;

                this->checkAllocator(allocator, "Remove: Container Check Failed", MA_SRC);

                allocator.commit();
            }

            this->StoreAllocator(allocator, this->getResourcePath(SBuf()<<"remove"<<(++cnt_)<<".dump"));
        }
        catch (...) {
            this->dump_name_ = Store(allocator);
            throw;
        }
    }

    virtual void replay(TestFn test_fn)
    {
        Allocator allocator;
        DefaultLogHandlerImpl logHandler(out());
        allocator.getLogger()->setHandler(&logHandler);

        LoadAllocator(allocator, this->dump_name_);

        Ctr ctr(&allocator, CTR_FIND, this->ctr_name_);

        test_fn(this, allocator, ctr);

        this->checkAllocator(allocator, "Insert: Container Check Failed", MA_SRC);
    }
};




}


#endif
