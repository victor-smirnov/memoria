
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

#include <memoria/core/hermes/map.hpp>
#include <memoria/core/hermes/document.hpp>

namespace memoria {
namespace hermes {

inline void Map<Varchar, Value>::assert_mutable()
{
    if (MMA_UNLIKELY(doc_->is_mutable())) {
        MEMORIA_MAKE_GENERIC_ERROR("Map<String, Value> is immutable");
    }
}

template <typename DT>
inline DataObjectPtr<DT> Map<Varchar, Value>::put_tv(U8StringView key, DTTViewType<DT> value)
{
    assert_not_null();
    assert_mutable();

    auto key_ptr = doc_->new_tv<Varchar>(key);
    auto value_ptr = doc_->new_tv<DT>(value);

    auto arena = doc_->arena();    
    map_->put(*arena, key_ptr->dt_ctr_, value_ptr->dt_ctr_);

    return value_ptr;
}


inline GenericMapPtr Map<Varchar, Value>::put_generic_map(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    auto key_ptr = doc_->new_tv<Varchar>(key);
    auto value_ptr = doc_->new_map();

    auto arena = doc_->arena();
    map_->put(*arena, key_ptr->dt_ctr_, value_ptr->map_);

    return value_ptr;
}

inline GenericArrayPtr Map<Varchar, Value>::put_generic_array(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    auto key_ptr = doc_->new_tv<Varchar>(key);
    auto value_ptr = doc_->new_array();

    auto arena = doc_->arena();
    map_->put(*arena, key_ptr->dt_ctr_, value_ptr->array_);

    return value_ptr;
}


inline void Map<Varchar, Value>::remove(U8StringView key)
{
    assert_not_null();
    assert_mutable();

    map_->remove(*(doc_->arena()), key);
}

inline void Map<Varchar, Value>::do_stringify(std::ostream& out, DumpFormatState state, DumpState& dump_state)
{
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

inline void Map<Varchar, Value>::put(StringValuePtr name, ValuePtr value) {
    assert_not_null();
    assert_mutable();

    auto arena = doc_->arena();
    map_->put(*arena, name->dt_ctr_, value->addr_);
}

}}
