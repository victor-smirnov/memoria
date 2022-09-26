
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

#pragma once

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/arena/string.hpp>

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/common.hpp>


namespace memoria {
namespace hermes {

class HermesDocView;
class HermesDoc;

template <typename DT>
class Datatype: public HoldingView {
public:
    using ArenaDTContainer = arena::ArenaDataTypeContainer<DT>;

    friend class HermesDoc;
    friend class HermesDocView;

protected:
    ArenaDTContainer* dt_ctr_;
public:
    Datatype() {}

    Datatype(void* dt_ctr, HermesDocView*, ViewPtrHolder* ptr_holder) noexcept :
        HoldingView(ptr_holder), dt_ctr_(reinterpret_cast<ArenaDTContainer*>(dt_ctr))
    {}

    DTTViewType<DT> view() const
    {
        assert_not_null();
        return dt_ctr_->view();
    }

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state)
    {
        dt_ctr_->stringify(out, state, dump_state);
    }

    bool is_simple_layout() {
        return true;
    }

private:

    void assert_not_null() const
    {
        if (MMA_UNLIKELY(dt_ctr_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Datatype<Varchar> is null");
        }
    }
};

namespace detail {

template <typename DT>
struct ValueCastHelper {
    static ViewPtr<Datatype<DT>> cast_to(void* addr, HermesDocView* doc, ViewPtrHolder* ref_holder) noexcept {
        return ViewPtr<Datatype<DT>>(Datatype<DT>(
            reinterpret_cast<arena::ArenaDataTypeContainer<DT>*>(addr),
            doc,
            ref_holder
        ));
    }
};

}

}}
