// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <memoria/allocators/persistent-inmem/factory.hpp>
#include <memoria/core/container/metadata_repository.hpp>
#include <memoria/core/tools/file.hpp>

namespace memoria {

template <>
struct CtrNameDeclarator<0>: TypeDef<Map<UUID, UUID>> {};

}


