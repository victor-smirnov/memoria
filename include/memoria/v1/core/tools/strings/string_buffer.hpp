
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


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
    SBuf& operator<<(const T& value)
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
