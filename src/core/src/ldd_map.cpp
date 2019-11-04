// Copyright 2019 Victor Smirnov
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


//#define BOOST_SPIRIT_X3_DEBUG 1

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/linked/document/linked_document.hpp>

namespace memoria {
namespace v1 {

void LDDMap::do_dump(std::ostream& out, LDDumpFormatState state, LDDumpState& dump_state) const
{
    if (size() > 0)
    {
        out << "{" << state.nl_start();

        bool first = true;

        state.push();
        for_each([&](auto kk, auto vv){
            if (MMA1_LIKELY(!first)) {
                out << "," << state.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);

            out << "'" << kk << "': ";
            vv.dump(out, state, dump_state);
        });
        state.pop();

        out << state.nl_end();

        state.make_indent(out);
        out << "}";
    }
    else {
        out << "{}";
    }
}


}}
