
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_VECTOR_MAP_NAMES_HPP
#define _MEMORIA_MODELS_VECTOR_MAP_NAMES_HPP

#include <memoria/core/container/names.hpp>

namespace memoria    {
namespace vector_map {

template <Int Indexes>
struct VMset {
	static const Int Code = 56721;
};

struct CtrApiName {};
struct ItrApiName {};

template <typename Types>
struct VectorMapCtrTypes: CtrTypesT<Types> {};

template <typename Types>
struct VectorMapIterTypes: IterTypesT<Types> {};




}
}

#endif
