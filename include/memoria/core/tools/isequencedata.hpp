
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_ISEQUENCEDATA_HPP
#define _MEMORIA_CORE_TOOLS_ISEQUENCEDATA_HPP


#include <memoria/core/tools/idata.hpp>

namespace memoria    {
namespace vapi       {


template <typename T, Int BitsPerSymbol>
struct ISequenceDataSource: IDataBase {
	virtual ~ISequenceDataSource() throw() {}

	virtual SizeT get(T* buffer, SizeT start, SizeT length)   					= 0;
};


template <typename T, Int BitsPerSymbol>
struct ISequenceDataTarget: IDataBase {
	virtual ~ISequenceDataTarget() throw() {}

	virtual SizeT put(T* buffer, SizeT start, SizeT length)   					= 0;
};


}
}



#endif
