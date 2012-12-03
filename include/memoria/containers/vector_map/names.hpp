
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_VECTOR_MAP_NAMES_HPP
#define _MEMORIA_MODELS_VECTOR_MAP_NAMES_HPP

#include <memoria/core/container/names.hpp>

#include <memoria/core/types/typehash.hpp>

namespace memoria    {
namespace vector_map {

template <Int Indexes>
struct VMSetCtr {};

struct CtrApiName {};
struct ItrApiName {};

template <typename Types>
struct VectorMapCtrTypes: CtrTypesT<Types> {};

template <typename Types>
struct VectorMapIterTypes: IterTypesT<Types> {};

}

template <Int Indexes>
class TypeHash<vector_map::VMSetCtr<Indexes> > {
public:
    static const UInt Value = 1000 + Indexes;
};


}

#endif
