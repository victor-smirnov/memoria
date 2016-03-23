
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/v1/core/tools/strings/strings.hpp>

#include <sstream>
#include <vector>

namespace memoria {
namespace v1 {



class SBuf {

    std::stringstream buffer_;

public:

    SBuf() {}
    SBuf(const SBuf& other): buffer_(other.buffer_.str()) {}

    std::stringstream& buffer() {
        return buffer_;
    }

    const std::stringstream& buffer() const {
        return buffer_;
    }

    String str() const {
        return buffer_.str();
    }

    operator String() const {
        return buffer_.str();
    }

    template <typename T>
    SBuf& operator<<(T value)
    {
        buffer_<<value;
        return *this;
    }

    template <typename T>
    SBuf& operator<<(const std::vector<T>& values)
    {
        buffer_<<"[";
        for (Int c = 0; c < values.size(); c++) {
            buffer_<<values[c];

            if (c != values.size() - 1) {
                buffer_<<", ";
            }
        }
        buffer_<<"]";

        return *this;
    }
};




}}
