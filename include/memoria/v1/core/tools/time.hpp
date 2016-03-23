// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/core/types/types.hpp>

namespace memoria {
namespace v1 {



BigInt  getTimeInMillis();
String FormatTime(BigInt millis);

}}