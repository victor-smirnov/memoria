
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/metadata/tools.hpp>
#include <memoria/metadata/page.hpp>
#include <memoria/core/tools/dump.hpp>

#include <sstream>
#include <string>

namespace memoria {

using namespace std;

void Expand(ostream& os, Int level)
{
    for (Int c = 0; c < level; c++) {
    	os<<" ";
    }
}




void dumpPage(PageMetadata* meta, const Page* page, std::ostream& out)
{
    TextPageDumper dumper(out);

    meta->getPageOperations()->generateDataEvents(page->Ptr(), DataEventsParams(), &dumper);
}


void dumpPageData(PageMetadata* meta, const void* page, std::ostream& out)
{
    TextPageDumper dumper(out);

    meta->getPageOperations()->generateDataEvents(page, DataEventsParams(), &dumper);
}


}


