
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
#include <memoria/core/hermes/data_object.hpp>

namespace memoria {
namespace hermes {

template <>
class Map<Varchar, Value>: public HoldingView {
public:
    using KeyT = DataObject<Varchar>::ArenaDTContainer;

    using ArenaMap = arena::Map<KeyT*, void*>;
protected:
    ArenaMap* map_;
    HermesDocView* doc_;

    friend class HermesDoc;
    friend class HermesDocView;
    friend class Value;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    friend class DocumentBuilder;

public:
    Map() noexcept : map_(), doc_() {}

    Map(void* map, HermesDocView* doc, ViewPtrHolder* ptr_holder) noexcept :
        HoldingView(ptr_holder),
        map_(reinterpret_cast<ArenaMap*>(map)), doc_(doc)
    {}

    ValuePtr as_value() {
        return ValuePtr(Value(map_, doc_, ptr_holder_));
    }

    uint64_t size() const {
        assert_not_null();
        return map_->size();
    }

    ValuePtr get(U8StringView key) const
    {
        assert_not_null();
        auto res = map_->get(key);

        if (res) {
            return ValuePtr(Value(
                res->get(), doc_, ptr_holder_
            ));
        }

        return ValuePtr();
    }


    template <typename DT>
    DataObjectPtr<DT> put_tv(U8StringView key, DTTViewType<DT> value);

    GenericMapPtr put_generic_map(U8StringView key);
    GenericArrayPtr put_generic_array(U8StringView key);

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
        assert_not_null();
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

    void remove(U8StringView key);

protected:
    void put(StringValuePtr name, ValuePtr value);
private:
    void do_stringify(std::ostream& out, DumpFormatState state, DumpState& dump_state);


    void assert_not_null() const
    {
        if (MMA_UNLIKELY(map_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Map<Varchar, Value> is null");
        }
    }

    void assert_mutable();
};



}}
