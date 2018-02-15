
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

#include <memoria/v1/core/exceptions/memoria.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>

namespace memoria {
namespace v1 {

using namespace std;

//class DispatchException: public Exception {
//    int32_t code1_;
//    int32_t code2_;
//public:

//    DispatchException(const char* source, const string &message, int32_t code1 = 0, int32_t code2 = 0):
//                Exception(source, message), code1_(code1), code2_(code2) {}

//    DispatchException(const char* source, const SBuf &message, int32_t code1 = 0, int32_t code2 = 0):
//                    Exception(source, message.str()), code1_(code1), code2_(code2) {}

//    DispatchException(const char* source, int32_t code1, int32_t code2 = -1):
//                Exception(source, "Invalid dispatch"), code1_(code1), code2_(code2) {}

//    int32_t code1() const {
//        return code1_;
//    }

//    int32_t code2() const {
//        return code2_;
//    }
//};


}}
