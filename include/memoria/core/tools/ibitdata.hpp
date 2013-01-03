
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_IBITDATA_HPP
#define _MEMORIA_CORE_TOOLS_IBITDATA_HPP


#include <memoria/core/tools/idata.hpp>

namespace memoria    {
namespace vapi       {

template <typename T>
struct IBitData: IData<T> {
	virtual T prefix() = 0;
	virtual T suffix() = 0;
};

}
}



#endif
