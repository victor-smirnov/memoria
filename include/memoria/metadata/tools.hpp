
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/metadata/group.hpp>
#include <memoria/metadata/page.hpp>
#include <memoria/core/container/pages.hpp>


#include <iostream>

namespace memoria    {


void dumpPage(PageMetadata* meta, const Page* page, std::ostream& out);
void dumpPageData(PageMetadata* meta, const void* page, std::ostream& out);

}
