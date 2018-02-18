
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

#include <memoria/v1/core/config.hpp>
#include <cstdint>
#include <ostream>

namespace memoria {
namespace v1 {

typedef __uint64_t         UInt32128;
typedef __int64_t          Int128;

}

namespace std {

//ostream& operator<<(ostream& out, const v1::Int128& value) {
//    out<<value;
//    return out;
//}
//
//ostream& operator<<(ostream& out, const v1::UInt32128& value) {
//    out<<value;
//    return out;
//}

}}