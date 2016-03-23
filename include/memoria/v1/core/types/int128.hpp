
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <cstdint>
#include <ostream>

namespace memoria {
namespace v1 {

typedef __uint64_t         UInt128;
typedef __int64_t          Int128;

}

namespace std {

//ostream& operator<<(ostream& out, const v1::Int128& value) {
//    out<<value;
//    return out;
//}
//
//ostream& operator<<(ostream& out, const v1::UInt128& value) {
//    out<<value;
//    return out;
//}

}}