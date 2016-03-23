
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <memoria/v1/core/container/names.hpp>

namespace memoria {

template <typename Types>
struct CtrWrapperTypes: CtrTypesT<Types> {};

template <typename Types>
struct IterWrapperTypes: IterTypesT<Types> {};

}
