// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_HPP
#define _MEMORIA_HPP

#include <memoria/allocators/persistent-inmem/factory.hpp>
#include <memoria/core/tools/file.hpp>

namespace memoria {

template <>
struct CtrNameDeclarator<0>: TypeDef<Map<UUID, UUID>> {};

}

#endif


