// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/tools/strings/string.hpp>
#include <memoria/core/types/types.hpp>

namespace memoria   {



BigInt  getTimeInMillis();
String FormatTime(BigInt millis);

}
