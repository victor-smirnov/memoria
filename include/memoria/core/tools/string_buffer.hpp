
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
    SBuf(const memoria::SBuf& other): buffer_(other.buffer_.str()) {}

    stringstream& buffer() {
        return buffer_;
    }

    const stringstream& buffer() const {
        return buffer_;
    }

    String str() const {
        return buffer_.str();
    }

    template <typename T>
    SBuf& operator<<(const T& value)
    {
        buffer_<<value;
        return *this;
    }
};




}



#endif

