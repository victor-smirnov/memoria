
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


#include <memoria/core/hermes/document.hpp>
#include <memoria/core/hermes/value.hpp>

namespace memoria::hermes {

pool::SharedPtr<HermesDocView> HermesDocView::make_pooled(ObjectPools& pool) {
    return get_reusable_shared_instance<HermesDocImpl>(pool);
}

pool::SharedPtr<HermesDocView> HermesDocView::make_new(size_t initial_capacity) {
    return TL_allocate_shared<HermesDocImpl>(initial_capacity);
}


StringEscaper& StringEscaper::current() {
    static thread_local StringEscaper escaper;
    return escaper;
}

std::ostream& operator<<(std::ostream& out, ValuePtr ptr) {
    out << ptr->to_string();
    return out;
}

std::ostream& operator<<(std::ostream& out, DatatypePtr ptr) {
    out << ptr->to_string();
    return out;
}

}
