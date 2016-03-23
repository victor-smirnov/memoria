
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.




#include <memoria/v1/metadata/tools.hpp>
#include <memoria/v1/metadata/page.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <sstream>
#include <string>

namespace memoria {
namespace v1 {

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


}}