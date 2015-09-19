
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BT_TEST_BASE_HPP_
#define MEMORIA_TESTS_BT_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <functional>
#include <memory>

namespace memoria {

using namespace std;

template <
	typename ContainerTypeName,
    typename AllocatorType,
	typename Profile
>
class BTTestBase: public TestTask {

    typedef BTTestBase<
                Profile,
                AllocatorType,
                ContainerTypeName
    >                                                                           MyType;

    typedef TestTask                                                            Base;

protected:
    typedef typename CtrTF<Profile, ContainerTypeName>::Type                    Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;
    typedef typename Ctr::Accumulator                                           Accumulator;

    typedef AllocatorType                                                       Allocator;

    using AllocatorSPtr = std::shared_ptr<Allocator>;
    using AllocatorUPtr = std::unique_ptr<Allocator>;

    AllocatorSPtr allocator_;

public:

    BTTestBase(StringRef name):
        TestTask(name)
    {
        Ctr::initMetadata();
    }

    AllocatorSPtr& allocator() {
    	return allocator_;
    }

    const AllocatorSPtr& allocator() const {
    	return allocator_;
    }

    virtual ~BTTestBase() throw() {}

    virtual void createAllocator(AllocatorSPtr& allocator_) = 0;

    virtual Ctr createCtr() {
    	return Ctr(allocator_.get(), CTR_CREATE);
    }

    virtual Ctr createOrFindCtr(BigInt name) {
    	return Ctr(allocator_.get(), CTR_CREATE | CTR_FIND, name);
    }

    virtual void setUp() {
    	createAllocator(allocator_);

    	MEMORIA_ASSERT_NOT_NULL(allocator_.get());
    }

    virtual void tearDown()
    {
    	allocator_ = AllocatorSPtr();
    }

    virtual void storeAllocator(String file_name)
    {
    	unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
    	allocator_->store(out.get());
    }
};

}


#endif
