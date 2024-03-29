
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

#include <memoria/core/hermes/typed_value.hpp>

namespace memoria {
namespace hermes {


void TypedValueView::stringify(std::ostream& out,
               DumpFormatState& state) const
{
    auto ctr = constructor();
    auto type = this->datatype();

    if (MMA_LIKELY(!ctr.is_varchar()))
    {
        auto space = state.cfg().spec().space();
        out << '@';
        type.stringify(out, state);
        out << space << "=" << space;
        ctr.stringify(out, state);
    }
    else {
        ctr.stringify(out, state);
        out << '@';
        type.stringify(out, state);
    }
}

}}
