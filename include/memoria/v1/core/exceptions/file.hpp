
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/exceptions/memoria.hpp>
#include <string>

namespace memoria {
namespace v1 {


using namespace std;

class MEMORIA_API FileException: public Exception {

public:
    FileException(const char* source, StringRef message):
                Exception(source, message) {}

    FileException(const char* source, const SBuf& message):
                    Exception(source, message.str()) {}
};


}}