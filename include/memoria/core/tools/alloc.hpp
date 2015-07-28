
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_TOOLS_ALLOC_HPP_
#define MEMORIA_CORE_TOOLS_ALLOC_HPP_

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <malloc.h>
#include <memory>
#include <type_traits>


namespace memoria {

template <typename T>
using FreeUniquePtr = std::unique_ptr<T, decltype(free)*>;


template <typename T>
FreeUniquePtr<T> AllocateUnique(size_t block_size, const char* source = MA_SRC)
{
	T* ptr = T2T<T*>(malloc(block_size));

	if (ptr != nullptr)
	{
		return FreeUniquePtr<T>(ptr, free);
	}
	else {
		throw new OOMException(source);
	}
}

}


#endif

