
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_TESTS_FILEALLOC_CTR_TEST_HPP_
#define MEMORIA_TESTS_FILEALLOC_CTR_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include <memoria/allocators/file/factory.hpp>

#include "file_alloc_test_base.hpp"

namespace memoria {

class FileAllocatorCtrTest: public FileAllocatorTestBase {
    typedef FileAllocatorTestBase                                               Base;
    typedef FileAllocatorCtrTest                                                MyType;

protected:
    typedef typename Base::Allocator                                            Allocator;

    typedef typename FCtrTF<Vector<Int>>::Type                                  Ctr;

public:

    FileAllocatorCtrTest(StringRef name): Base(name)
    {
        Ctr::initMetadata();
    }


};

}


#endif
