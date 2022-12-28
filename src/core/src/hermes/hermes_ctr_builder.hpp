
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

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/hermes/path/types.h>
#include <memoria/core/datatypes/varchars/varchars.hpp>
#include <memoria/core/strings/u8_string_view.hpp>


namespace memoria::hermes {

struct HermesCtrBuilderCleanup;

class HermesCtrBuilder {

    using UnicodeChar = path::UnicodeChar;

    ArenaBuffer<char> string_buffer_;
    HermesCtr doc_;

    ska::flat_hash_map<U8String, Datatype> type_registry_;
    std::unordered_map<U8StringView, DTView<Varchar>, arena::DefaultHashFn<U8StringView>> string_registry_;

    size_t refs_{};

    friend struct HermesCtrBuilderCleanup;

    HermesCtrBuilder()
    {}
public:

    auto& string_buffer() {return string_buffer_;}
    const auto& string_buffer() const {return string_buffer_;}

    HermesCtr& doc() {
        return doc_;
    }

    DTView<Varchar> resolve_string(U8StringView strv)
    {
        auto ii = string_registry_.find(strv);
        if (ii != string_registry_.end()) {
            return ii->second;
        }
        return DTView<Varchar>{};
    }

    U8StringView to_string_view(Span<const char> span) {
        return U8StringView(span.data(), span.size());
    }

    void append_char(UnicodeChar utf32Char)
    {
        auto outIt = std::back_inserter(string_buffer_);
        boost::utf8_output_iterator<decltype(outIt)> utf8OutIt(outIt);
        *utf8OutIt++ = utf32Char;
    }

    void clear_string_buffer() {
        string_buffer_.clear();
    }

    bool is_string_buffer_empty() const {
        return string_buffer_.size() == 0;
    }

    Object new_varchar()
    {
        auto span = string_buffer_.span();
        U8StringView sv = to_string_view(span);
        DTView<Varchar> str = resolve_string(sv);

        if (str.is_empty()) {
            str = doc_.make_t<Varchar>(sv).as_varchar();
            string_registry_[str] = str;
        }

        return doc_.import_object(str);
    }

    Object new_varchar(U8StringView sv)
    {
        auto str = resolve_string(sv);

        if (str.is_empty()) {
            str = doc_.make_t<Varchar>(sv).as_varchar();
            string_registry_[str] = str;
        }

        return doc_.import_object(str);
    }

    auto new_parameter(U8StringView name) {
        return doc_.new_parameter(name);
    }

    auto new_identifier()
    {
        return new_varchar();
    }

    auto new_boolean(bool v) {
        return HermesCtrView::wrap_dataobject<Boolean>(v);
    }

    void set_ctr_root(Object value) {
        doc_.set_root(value);
    }

    auto make_datatype(Object id) {
        return doc_.make_datatype(id.as_varchar());
    }

    void add_type_decl_param(Datatype& dst, Object param) {
        dst.append_type_parameter(param);
    }

    void add_type_decl_ctr_arg(Datatype& dst, Object ctr_arg) {
        dst.append_constructor_argument(ctr_arg);
    }

    TypedValue new_typed_value(Datatype type_decl, Object constructor)
    {
        return doc_.new_typed_value(type_decl, constructor);
    }

    void add_type_directory_entry(U8StringView id, Datatype datatype)
    {
        auto ii = type_registry_.find(id);
        if (ii == type_registry_.end()) {
            type_registry_[id] = datatype;
        }
    }

    Datatype resolve_typeref(U8StringView typeref)
    {
        auto ii = type_registry_.find(typeref);
        if (ii != type_registry_.end())
        {
            return ii->second;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("No datatype with reference '{}' in the registry", typeref).do_throw();
        }
    }

    bool check_typeref(U8StringView typeref)
    {
        auto ii = type_registry_.find(typeref);
        return (ii != type_registry_.end());
    }


    MMA_NODISCARD auto append_entry(ObjectMap& map, U8StringView name, const Object& value) {
        return map.put(name, value);
    }


    static void enter() {
        current().ref();
    }

    static void enter(HermesCtr ctr) {
        current().do_enter(std::move(ctr));
    }

    static HermesCtrBuilder& current()
    {
        thread_local HermesCtrBuilder ii_holder;
        return ii_holder;
    }

    void exit() {
        if (--refs_ == 0) {
            reset();
        }
    }

    void reset() {
        doc_.reset();
        string_buffer_.reset();
        refs_ = 0;
    }

private:
    void ref() {
        if (refs_++ == 0) {
            doc_ = HermesCtrView::make_pooled();
        }
    }

    void do_enter(HermesCtr&& ctr) {
        doc_ = std::move(ctr);
        refs_ = 1;
    }
};

struct HermesCtrBuilderCleanup {
    ~HermesCtrBuilderCleanup() noexcept {
        HermesCtrBuilder::current().reset();
    }
};

static inline HermesCtr current_ctr() {
    return HermesCtrBuilder::current().doc();
}

}
