
// Copyright 2022 Victor Smirnov
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


#include <memoria/core/ssrle/ssrle.hpp>
#include <memoria/core/strings/format.hpp>

//#include <memoria/memoria.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include <bitset>
#include <iostream>

//void InitCoreLDDatatypes();

namespace memoria {
void InitMemoriaCoreExplicit() ;


LDDArrayView get_or_add_array(LDDMapView map, const U8String& name)
{
    auto res = map.get(name);

    if (res.is_initialized()) {
        return res.get().as_array();
    }

    return map.set_array(name);
}

}

using namespace memoria;

int main(int, char**)
{
    //InitCoreLDDatatypes();

    InitMemoriaCoreExplicit();

    LDDocument doc;

    LDDMapView map = doc.set_map();

    LDDArrayView profiles = get_or_add_array(map, "active_profiles");

    std::vector<std::string> enabled_profiles {"Profile1", "Profile2", "Profile3"};

    for (const auto& profile: enabled_profiles) {
        profiles.add_varchar(profile);

        println("doc: {}", doc.to_pretty_string());
    }

    println("Profiles: {}", doc.to_pretty_string());

    println("doc: {}", doc.to_pretty_string());

}
