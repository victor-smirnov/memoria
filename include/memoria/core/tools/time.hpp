// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_TIME_HPP_
#define MEMORIA_CORE_TOOLS_TIME_HPP_

#include <memoria/core/tools/strings/string.hpp>
#include <memoria/core/types/types.hpp>

namespace memoria   {



BigInt  getTimeInMillis();
String FormatTime(BigInt millis);

}


#endif /* TERMINAL_HPP_ */
