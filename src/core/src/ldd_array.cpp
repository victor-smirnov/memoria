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


#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/linked/document/linked_document.hpp>

namespace memoria {
namespace v1 {

LDDMap LDDArray::add_map()
{
    LDDMap map = doc_->make_mutable()->new_map();
    array_.push_back(map.map_.ptr());
    return map;
}


void LDDArray::do_dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    if (size() > 0)
    {
        out << "[" << state.nl_start();

        bool first = true;

        state.push();
        for_each([&](auto vv){
            if (MMA1_LIKELY(!first)) {
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


LDDMap LDDArray::set_map(size_t idx)
{
    LDDMap vv = doc_->make_mutable()->new_map();
    array_.access_checked(idx) = vv.map_.ptr();
    return vv;
}


ld_::LDPtr<LDDArray::Array::State> LDDArray::deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const
{
    ld_::DeepCopyHelper<ld_::LDDValueDeepCopyHelperBase<LDDArray>> helper(mapping, doc_, tgt);
    return array_.deep_copy_to(tgt->arena_.make_mutable(), helper);
}

}}
