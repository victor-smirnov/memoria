
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/names.hpp>

namespace memoria     {
namespace composite   {

template <typename Types>
struct CompositeCtrTypesT: CtrTypesT<Types>         {};

template <typename Types>
struct CompositeIterTypesT: IterTypesT<Types>       {};



template <typename Types>
using CompositeCtrTypes = CtrTypesT<CompositeCtrTypesT<Types>>;

template <typename Types>
using CompositeIterTypes = CtrTypesT<CompositeIterTypesT<Types>>;


}
}

