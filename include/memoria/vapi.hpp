
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_VAPI_API_HPP
#define	_MEMORIA_VAPI_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/vapi/models.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/config.hpp>

namespace memoria    {
namespace vapi       {

MEMORIA_API extern void InitTypeSystem(int argc, const char** argv, const char** envp, bool read_config_files = true);
MEMORIA_API extern void DestroyTypeSystem();

}
}
#endif


