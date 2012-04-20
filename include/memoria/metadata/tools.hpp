
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_TOOLS_HPP
#define _MEMORIA_VAPI_METADATA_TOOLS_HPP

#include <memoria/metadata/field.hpp>
#include <memoria/metadata/id.hpp>
#include <memoria/metadata/bitmap.hpp>
#include <memoria/metadata/number.hpp>
#include <memoria/metadata/group.hpp>
#include <memoria/metadata/page.hpp>
#include <memoria/metadata/map.hpp>

#include <iostream>

namespace memoria    {
namespace vapi       {

struct Page;

void Expand(std::ostream& os, Int level);

//void DumpField(FieldMetadata* field, Page* page, std::ostream &out, Int level, Int idx);
//void DumpMap(MapMetadata* group, Page* page, std::ostream &out, Int level, Int idx);
//void DumpData(MetadataGroup* group, Page* page, std::ostream &out, Int level, Int idx);
//void DumpGroup(MetadataGroup* group, Page* page, std::ostream &out, Int level, Int idx, Int size = -1);

void DumpPage(PageMetadata* meta, Page* page, std::ostream& out);

}}

#endif
