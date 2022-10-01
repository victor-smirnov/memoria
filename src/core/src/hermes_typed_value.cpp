
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


void TypedValue::stringify(std::ostream& out,
               DumpFormatState& state,
               DumpState& dump_state)
{

    auto ctr = constructor();
    auto type = this->datatype();

    //auto ref = dump_state.resolve_type_id(type->state_.get());

    if (MMA_LIKELY(!ctr->is_varchar()))
    {
        out << '@';
        //if (!ref)
        {
            type->stringify(out, state, dump_state);
        }
//        else {
//            out << ref.get();
//        }
        out << " = ";
        ctr->stringify(out, state, dump_state);
    }
    else {
        ctr->stringify(out, state, dump_state);
        out << '@';
        //if (!ref)
        {
            type->stringify(out, state, dump_state);
        }
//        else {
//            out << '#' << ref.get();
//        }
    }
}

}}
