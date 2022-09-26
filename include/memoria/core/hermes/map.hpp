
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

#include <memoria/core/arena/map.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/datatype.hpp>
#include <memoria/core/hermes/common.hpp>

namespace memoria {
namespace hermes {

template <>
class Map<Varchar, Value>: public HoldingView {
public:
    using KeyT = Datatype<Varchar>::ArenaDTContainer;

    using ArenaMap = arena::Map<KeyT*, void*>;
protected:
    ArenaMap* map_;
    HermesDocView* doc_;
public:
    Map() noexcept : map_(), doc_() {}

    Map(void* map, HermesDocView* doc, ViewPtrHolder* ptr_holder) noexcept :
        HoldingView(ptr_holder),
        map_(reinterpret_cast<ArenaMap*>(map)), doc_(doc)
    {}

    uint64_t size() const {
        assert_not_null();
        return map_->size();
    }

    ViewPtr<Value> get(U8StringView key) const
    {
        assert_not_null();
        auto res = map_->get(key);

        if (res) {
            return ViewPtr<Value>(Value(
                res->get(), doc_, ptr_holder_
            ));
        }

        return ViewPtr<Value>();
    }

    ViewPtr<Datatype<Varchar>> put_varchar(U8StringView key, U8StringView value);

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state)
    {
        if (state.indent_size() == 0 || !is_simple_layout()) {
            do_stringify(out, state, dump_state);
        }
        else {
            DumpFormatState simple_state = state.simple();
            do_stringify(out, simple_state, dump_state);
        }
    }


    void for_each(std::function<void(U8StringView, ViewPtr<Value>)> fn) const
    {
        map_->for_each([&](const auto& key, const auto& value){
            U8StringView kk = key->view();
            fn(kk, ViewPtr<Value>{Value(value.get(), doc_, ptr_holder_)});
        });
    }

    bool is_simple_layout() noexcept
    {
        if (size() > 2) {
            return false;
        }

        bool simple = true;

        for_each([&](auto, auto vv){
            simple = simple && vv->is_simple_layout();
        });

        return simple;
    }



private:
    void do_stringify(std::ostream& out, DumpFormatState state, DumpState& dump_state) {
        if (size() > 0)
        {
            out << "{" << state.nl_start();

            bool first = true;

            state.push();
            for_each([&](auto kk, auto vv){
                if (MMA_LIKELY(!first)) {
                    out << "," << state.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);

                U8StringView kk_escaped = StringEscaper::current().escape_quotes(kk);

                out << "'" << kk_escaped << "': ";

                StringEscaper::current().reset();

                vv->stringify(out, state, dump_state);
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


    void assert_not_null() const
    {
        if (MMA_UNLIKELY(map_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Map<Varchar, Value> is null");
        }
    }

    void assert_mutable();
};

}}
