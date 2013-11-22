
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_FILEALLOC_RANDOMACCESSLIST_TEST_BASE_HPP_
#define MEMORIA_TESTS_FILEALLOC_RANDOMACCESSLIST_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>

#include "../shared/abstract_sequence_test_base.hpp"

#include <functional>

namespace memoria {

using namespace std;

template <
    typename ContainerTypeName,
    typename MemBuffer
>
class FARandomAccessListTestBase: public AbstractSequenceTestBase<
	FileProfile<>,
	typename GenericFileAllocator::WalkableAllocator,
	ContainerTypeName,
	MemBuffer
> {

    typedef FARandomAccessListTestBase<
                ContainerTypeName,
                MemBuffer
    >                                                                           MyType;

    typedef AbstractSequenceTestBase<
    		FileProfile<>,
    		typename GenericFileAllocator::WalkableAllocator,
    		ContainerTypeName,
    		MemBuffer
    >                                                            				Base;

protected:
    typedef typename FCtrTF<ContainerTypeName>::Type                            Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;
    typedef typename Ctr::Accumulator											Accumulator;

    typedef GenericFileAllocator												Allocator;

    bool clear_cache_ = true;

    OpenMode mode_ = OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC;

    typedef typename Base::TestFn                     							TestFn;

public:

    FARandomAccessListTestBase(StringRef name):
        Base(name)
    {
        MEMORIA_ADD_TEST_PARAM(clear_cache_);
    }

    virtual ~FARandomAccessListTestBase() throw() {}


    virtual String Store(Allocator& allocator) const
    {
    	String new_name = allocator.file_name() + ".valid";

    	File file(allocator.file_name());
    	file.copy(new_name);

    	allocator.commit();

    	return new_name;
    }


    virtual void testInsert(TestFn test_fn)
    {
    	typename Allocator::Cfg cfg;

    	cfg.pages_buffer_size(1024);

        Allocator allocator(this->getResourcePath("insert.db"), mode_, cfg);
        DefaultLogHandlerImpl logHandler(this->out());
        allocator.getLogger()->setHandler(&logHandler);
        allocator.getLogger()->level() = Logger::ERROR;

        Ctr ctr(&allocator);
        this->ctr_name_ = ctr.name();

        allocator.commit();

        try {
            while (ctr.size() < this->size_)
            {
                test_fn(this, allocator, ctr);

                this->out()<<"Size: "<<ctr.size()<<endl;

                this->checkAllocator(allocator, "Insert: Container Check Failed", MA_SRC);

                allocator.flush();

                if (clear_cache_) {
                	allocator.clearCache();
                }
            }
        }
        catch (...) {
        	this->dump_name_ = Store(allocator);
            throw;
        }
    }


    virtual void testRemove(TestFn test_fn)
    {
    	typename Allocator::Cfg cfg;

    	cfg.pages_buffer_size(1024);

        Allocator allocator(this->getResourcePath("remove.db"), mode_, cfg);
        DefaultLogHandlerImpl logHandler(this->out());
        allocator.getLogger()->setHandler(&logHandler);

        Ctr ctr(&allocator);
        this->ctr_name_ = ctr.name();

        allocator.commit();

        try {

        	this->fillRandom(allocator, ctr, Base::size_);

            allocator.flush();

            while (ctr.size() > 0)
            {
                test_fn(this, allocator, ctr);

                this->out()<<"Size: "<<ctr.size()<<endl;

                this->checkAllocator(allocator, "Remove: Container Check Failed", MA_SRC);

                allocator.flush();

                if (clear_cache_) {
                	allocator.clearCache();
                }
            }
        }
        catch (...) {
        	this->dump_name_ = Store(allocator);
            throw;
        }
    }

    virtual void replay(TestFn test_fn)
    {
        Allocator allocator(this->dump_name_, OpenMode::RW);

        DefaultLogHandlerImpl logHandler(this->out());
        allocator.getLogger()->setHandler(&logHandler);

        Ctr ctr(&allocator, CTR_FIND, this->ctr_name_);

        test_fn(this, allocator, ctr);

        this->checkAllocator(allocator, "Insert: Container Check Failed", MA_SRC);
    }
};

}


#endif
