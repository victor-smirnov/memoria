
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_PACKED_CODECS_TEST_BASE_HPP_
#define MEMORIA_TESTS_PACKED_CODECS_TEST_BASE_HPP_

#include "../../tests_inc.hpp"


namespace memoria {

using namespace std;

template <typename ValueT>
class PackedCodecsTestBase: public TestTask {
	using Base = TestTask;
protected:

	static constexpr Int MEMBUF_SIZE = 1024*1024*64;


    using Value 	= ValueT;

public:

    using Base::getRandom;

    PackedCodecsTestBase(StringRef name): TestTask(name)
    {}

};

}


#endif

