
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_TOOLS_HPP
#define _MEMORIA_VAPI_METADATA_TOOLS_HPP


#include <memoria/metadata/group.hpp>
#include <memoria/metadata/page.hpp>
#include <memoria/core/container/pages.hpp>

#include <iostream>

namespace memoria    {
namespace vapi       {

void Expand(std::ostream& os, Int level);
void dumpPage(PageMetadata* meta, const Page* page, std::ostream& out);

}}

#endif
