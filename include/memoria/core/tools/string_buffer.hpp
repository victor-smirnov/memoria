
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_STRING_BUFFER_HPP_
#define MEMORIA_CORE_TOOLS_STRING_BUFFER_HPP_


#include <sstream>

namespace memoria {

using namespace std;

class SBuf {

	stringstream buffer_;

public:

	SBuf() {}

	stringstream& Buffer() {
		return buffer_;
	}

	const stringstream& Buffer() const {
		return buffer_;
	}

	String Str() const {
		return buffer_.str();
	}

	template <typename T>
	SBuf& operator<<(const T& value)
	{
		this->Buffer()<<value;
		return *this;
	}
};




}



#endif

