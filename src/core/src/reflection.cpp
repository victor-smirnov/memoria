
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



#include <memoria/core/strings/string.hpp>
#include <memoria/profiles/common/container_operations.hpp>

#include <memoria/profiles/common/metadata.hpp>
#include <memoria/profiles/impl/no_cow_profile.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif


namespace memoria {

std::ostream& operator<<(std::ostream& os, const IDValue& id) {
    os << id.str();
    return os;
}

//template class ProfileMetadata<NoCowProfile<>>;


namespace {

//ProfileMetadata<NoCowProfile<>>::Init init_def;

}

}
