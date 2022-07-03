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


#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <memoria/core/linked/document/linked_document.hpp>

namespace memoria {

LDDMapView LDDArrayView::add_map()
{
    return add_value<LDMap>().as_map();
}


void LDDArrayView::do_dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    if (size() > 0)
    {
        out << "[" << state.nl_start();

        bool first = true;

        state.push();
        for_each([&](auto vv){
            if (MMA_LIKELY(!first)) {
                out << "," << state.nl_middle();
            }
            else {
                first = false;
            }

            state.make_indent(out);
            vv.dump(out, state, dump_state);
        });
        state.pop();

        out << state.nl_end();

        state.make_indent(out);
        out << "]";
    }
    else {
        out << "[]";
    }
}


LDDMapView LDDArrayView::set_map(size_t idx)
{
    return set_value<LDMap>(idx).as_map();
}


ld_::LDPtr<LDDArrayView::Array::State> LDDArrayView::deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const
{
    ld_::DeepCopyHelper<ld_::LDDValueDeepCopyHelperBase<LDArray>> helper(mapping, doc_, tgt);
    return array_.deep_copy_to(tgt->arena_.make_mutable(), helper);
}

}
