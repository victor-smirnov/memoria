
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
            addr,
            doc,
            ref_holder
        ));
    }
};

}

}

namespace arena {

namespace detail {

template <typename DT>
struct DTFNVFixedSizeHasherHelper;

template <>
struct DTFNVFixedSizeHasherHelper<BigInt> {
    static void hash_to(FNVHasher<8>& hasher, int64_t value) {
        for (size_t c = 0; c < 8; c++) {
            hasher.append((uint8_t)(value >> (c * 8)));
        }
    }
};


template <>
struct DTFNVFixedSizeHasherHelper<Double> {
    static void hash_to(FNVHasher<8>& hasher, double value) {
        uint64_t u64val = value_cast<uint64_t>(value);
        for (size_t c = 0; c < 8; c++) {
            hasher.append((uint8_t)(u64val >> (c * 8)));
        }
    }
};


}



template <typename DT>
class ArenaDataTypeContainer<DT, FixedSizeDataTypeTag> {
    using ViewT = DTTViewType<DT>;

    ViewT value_;
public:
    ArenaDataTypeContainer(ViewT view):
        value_(view)
    {}

    const ViewT& view() const noexcept {
        return value_;
    }

    bool equals_to(const ViewT& view) const noexcept {
        return view() == view;
    }

    bool equals_to(const ArenaDataTypeContainer* other) const noexcept {
        return view() == other->view();
    }

    void hash_to(FNVHasher<8>& hasher) const noexcept {
        for (auto ch: view()) {
            hasher.append(ch);
        }
    }

    void stringify(std::ostream& out,
                   hermes::DumpFormatState& state,
                   hermes::DumpState& dump_state)
    {
        out << value_;
    }
};


}

}
