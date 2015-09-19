// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_TIME_HPP_
#define MEMORIA_CORE_TOOLS_TIME_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/strings.hpp>

namespace memoria   {

using namespace memoria::vapi;

BigInt  getTimeInMillis();
String FormatTime(BigInt millis);

}


#endif /* TERMINAL_HPP_ */
