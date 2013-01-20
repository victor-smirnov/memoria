
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
struct ISequenceDataSource: IDataSource<T> {
	virtual ~ISequenceDataSource() throw() {}

	virtual SizeT bits()	const									 			= 0;

	virtual void getPrefix(T* buffer, SizeT start, SizeT length)   				= 0;
	virtual void getSuffix(T* buffer)   										= 0;

	virtual SizeT getSuffixSize() const											= 0;
};


}
}



#endif
