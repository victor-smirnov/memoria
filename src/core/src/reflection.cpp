
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



#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/profiles/common/container_operations.hpp>

#include <memoria/v1/profiles/common/metadata.hpp>
#include <memoria/v1/profiles/default/default.hpp>

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif


namespace memoria {
namespace v1 {

std::ostream& operator<<(std::ostream& os, const IDValue& id) {
    os << id.str();
    return os;
}

template class ProfileMetadata<DefaultProfile<>>;


namespace {

ProfileMetadata<DefaultProfile<>>::Init init_def;

}

}}
