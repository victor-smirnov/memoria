
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/task.hpp>

namespace memoria {

using namespace std;

Task::~Task() throw () {
	try {
		for (auto p: parameters_) {
			delete p;
		}
	}
	catch (...) {

	}
}



}
