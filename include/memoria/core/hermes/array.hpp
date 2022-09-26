
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

#include <memoria/core/arena/vector.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/common.hpp>


namespace memoria {
namespace hermes {

template <typename V>
class Array;

template <>
class Array<Value>: public HoldingView {
public:
    using ArenaArray = arena::Vector<arena::RelativePtr<void>>;
protected:
    ArenaArray* array_;
    HermesDocView* doc_;
public:
    Array() noexcept:
        array_(), doc_()
    {}

    Array(void* array, HermesDocView* doc, ViewPtrHolder* ref_holder) noexcept:
        HoldingView(ref_holder),
        array_(reinterpret_cast<ArenaArray*>(array)), doc_(doc)
    {}

    uint64_t size() const {
        return array_->size();
    }

    ViewPtr<Value> get(uint64_t idx)
    {
        assert_not_null();

        if (idx < array_->size())
        {
            return ViewPtr<Value>(Value(array_->get(idx).get(), doc_, ptr_holder_));
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>: {} {}", idx, array_->size()).do_throw();
        }
    }

    ViewPtr<Datatype<Varchar>, true> append_varchar(U8StringView view);

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state)
    {
        assert_not_null();

        if (state.indent_size() == 0 || !is_simple_layout()) {
            do_stringify(out, state, dump_state);
        }
        else {
            DumpFormatState simple_state = state.simple();
            do_stringify(out, simple_state, dump_state);
        }
    }

    bool is_simple_layout()
    {
        assert_not_null();

        if (size() > 3) {
            return false;
        }

        bool simple = true;

        for_each([&](auto vv){
            simple = simple && vv->is_simple_layout();
        });

        return simple;
    }

    void for_each(std::function<void(ViewPtr<Value>)> fn) {
        assert_not_null();

        for (auto& vv: array_->span()) {
            fn(ViewPtr<Value>(Value(vv.get(), doc_, ptr_holder_)));
        }
    }

private:
    void assert_not_null() const {
        if (MMA_UNLIKELY(array_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Array<Value> is null");
        }
    }

    void assert_mutable();

    void do_stringify(std::ostream& out, DumpFormatState& state, DumpState& dump_state)
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
                vv->stringify(out, state, dump_state);
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
};


}}
