
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

#pragma once


#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/strings/u8_string.hpp>

#include <memoria/v1/core/mapped/arena.hpp>
#include <memoria/v1/core/mapped/mapped_string.hpp>
#include <memoria/v1/core/mapped/mapped_dyn_vector.hpp>
#include <memoria/v1/core/mapped/mapped_map.hpp>
#include <memoria/v1/core/mapped/mapped_set.hpp>

#include <memoria/v1/core/tools/optional.hpp>

#include <boost/variant.hpp>

namespace memoria {
namespace v1 {

struct SDN2Header {
    uint32_t version;
};

using SDN2Arena     = MappedArena<SDN2Header, uint32_t>;
using SDN2ArenaBase = typename SDN2Arena::ArenaBase;
using SDN2PtrHolder = SDN2ArenaBase::PtrHolderT;
using SDN2ValueTag  = uint16_t;

template <typename T>
using SDN2Ptr = typename SDN2Arena::template PtrT<T>;

namespace sdn2_ {

    using PtrHolder = SDN2ArenaBase::PtrHolderT;

    using ValueMap = MappedMap<
        SDN2Ptr<U8MappedString>,
        PtrHolder,
        SDN2ArenaBase,
        MappedPtrHashFn,
        MappedStringPtrEqualToFn
    >;

    using StringSet = MappedSet<
        SDN2Ptr<U8MappedString>,
        SDN2ArenaBase,
        MappedPtrHashFn,
        MappedStringPtrEqualToFn
    >;

    using MapState = typename ValueMap::State;

    using Array = MappedDynVector<PtrHolder, SDN2ArenaBase>;
    using ArrayState = typename Array::State;

    struct TypedValueState {
        PtrHolder type_ptr;
        PtrHolder value_ptr;
    };

    struct TypeDeclState;

    using TypeDeclPtr = SDN2Ptr<TypeDeclState>;

    struct TypeDeclState {
        SDN2Ptr<U8MappedString> name;
        SDN2Ptr<MappedVector<TypeDeclPtr>> type_params;
        SDN2Ptr<MappedVector<PtrHolder>> ctr_params;
    };

    struct DocumentState {
        PtrHolder value;
        SDN2Ptr<MappedVector<TypeDeclPtr>> type_directory;
        SDN2Ptr<sdn2_::StringSet::State> strings;
    };

    static inline void make_indent(std::ostream& out, size_t tabs) {
        for (size_t c = 0; c < tabs; c++) {
            out << "  ";
        }
    }
}



class SDN2Array;

class SDN2Value;
class SDN2Map;
class SDN2TypeDeclaration;
class SDN2TypeName;
class SDN2DataTypeParam;
class SDN2DataTypeCtrArg;
class SDN2Document;
class SDN2String;
class SDN2Identifier;

class SDN2DocumentBuilder;


template <typename T> struct SDN2ValueTraits;

template <>
struct SDN2ValueTraits<SDN2String> {
    static constexpr SDN2ValueTag ValueTag = 1;
};

template <>
struct SDN2ValueTraits<int64_t> {
    static constexpr SDN2ValueTag ValueTag = 2;
};

template <>
struct SDN2ValueTraits<double> {
    static constexpr SDN2ValueTag ValueTag = 3;
};

template <>
struct SDN2ValueTraits<SDN2Map> {
    static constexpr SDN2ValueTag ValueTag = 5;
};

template <>
struct SDN2ValueTraits<SDN2Array> {
    static constexpr SDN2ValueTag ValueTag = 6;
};

template <>
struct SDN2ValueTraits<SDN2TypeDeclaration> {
    static constexpr SDN2ValueTag ValueTag = 7;
};

template <>
struct SDN2ValueTraits<SDN2Identifier> {
    static constexpr SDN2ValueTag ValueTag = 8;
};



class SDN2Document {
    using ValueMap = sdn2_::ValueMap;
    using StringSet = sdn2_::StringSet;
    using Array = sdn2_::Array;

    SDN2Arena arena_;
    SDN2Ptr<sdn2_::DocumentState> doc_;

    friend class SDN2DocumentBuilder;
    friend class SDN2Map;
    friend class SDN2Array;
    friend class SDN2TypeName;
    friend class SDN2DataTypeParam;
    friend class SDN2DataTypeCtrArg;
    friend class SDN2TypeDeclaration;
    friend class SDN2Value;
    friend class SDN2String;
    friend class SDN2Identifier;

public:
    SDN2Document() {
        doc_ = allocate<sdn2_::DocumentState>(&arena_, sdn2_::DocumentState{0, 0, 0});
    }

    SDN2Value value() const;

    void set(U8StringView string);
    void set(int64_t value);
    void set(double value);

    SDN2Map set_map();
    SDN2Array set_array();

    static SDN2Document parse(U8StringView view) {
        return parse(view.begin(), view.end());
    }

    static SDN2Document parse(U8StringView::const_iterator start, U8StringView::const_iterator end);

    std::ostream& dump(std::ostream& out) const;

    SDN2String new_string(U8StringView view);
    SDN2Identifier new_identifier(U8StringView view);

    SDN2Value new_integer(int64_t value);
    SDN2Value new_double(double value);
    SDN2Array new_array(Span<SDN2Value> span);
    SDN2Array new_array();
    SDN2Map new_map();

    void set_value(SDN2Value value);

private:




    SDN2Ptr<U8MappedString> intern(U8StringView view);

    sdn2_::DocumentState* state() {
        return doc_.get(&arena_);
    }

    const sdn2_::DocumentState* state() const {
        return doc_.get(&arena_);
    }

    void set_tag(SDN2PtrHolder ptr, SDN2ValueTag tag)
    {
        SDN2Ptr<SDN2ValueTag> tag_ptr(ptr - sizeof(SDN2ValueTag));
        *tag_ptr.get(&arena_) = tag;
    }
};



class SDN2String {
    SDN2Document* doc_;
    SDN2Ptr<U8MappedString> string_;
public:
    SDN2String(): doc_(), string_({}) {}

    SDN2String(SDN2Document* doc, SDN2Ptr<U8MappedString> string):
        doc_(doc), string_(string)
    {}

    U8StringView view() const {
        return string_.get(&doc_->arena_)->view();
    }

    operator SDN2Value() const;
};

class SDN2Identifier {
    SDN2Document* doc_;
    SDN2Ptr<U8MappedString> string_;

public:

    SDN2Identifier(SDN2Document* doc, SDN2Ptr<U8MappedString> string):
        doc_(doc), string_(string)
    {}

    U8StringView view() const {
        return string_.get(&doc_->arena_)->view();
    }
};


class SDN2Value {
    SDN2Document* doc_;
    SDN2PtrHolder value_ptr_;
    SDN2ValueTag type_tag_;

    friend class SDN2Document;
    friend class SDN2Map;
    friend class SDN2Array;

public:
    SDN2Value() noexcept : doc_(), value_ptr_(), type_tag_() {}

    SDN2Value(SDN2Document* doc, SDN2PtrHolder value_ptr) noexcept :
        doc_(doc), value_ptr_(value_ptr),
        type_tag_(value_ptr ? get_tag(value_ptr) : 0)
    {}

    SDN2Map as_map() const noexcept;
    SDN2Array as_array() const noexcept;

    SDN2String as_string() const noexcept {
        return SDN2String(doc_, value_ptr_);
    }

    int64_t as_integer() const noexcept {
        return *SDN2Ptr<int64_t>(value_ptr_).get(&doc_->arena_);
    }

    double as_double() const noexcept {
        return *SDN2Ptr<double>(value_ptr_).get(&doc_->arena_);
    }

    bool is_null() const noexcept {
        return value_ptr_ == 0;
    }

    bool is_integer() const noexcept {
        return type_tag_ == SDN2ValueTraits<int64_t>::ValueTag;
    }

    bool is_double() const noexcept {
        return type_tag_ == SDN2ValueTraits<double>::ValueTag;
    }

    bool is_string() const noexcept {
        return type_tag_ == SDN2ValueTraits<SDN2String>::ValueTag;
    }

    bool is_array() const noexcept {
        return type_tag_ == SDN2ValueTraits<SDN2Array>::ValueTag;
    }

    bool is_map() const noexcept {
        return type_tag_ == SDN2ValueTraits<SDN2Map>::ValueTag;
    }

    SDN2ValueTag tag() const {
        return type_tag_;
    }

    void dump(std::ostream& out, size_t indent = 0) const;

private:

    SDN2ValueTag get_tag(SDN2PtrHolder ptr) const noexcept
    {
        SDN2Ptr<SDN2ValueTag> tag_ptr(ptr - sizeof(SDN2ValueTag));
        return *tag_ptr.get(&doc_->arena_);
    }
};




class SDN2Array {
    using Array = sdn2_::Array;
    using ValueMap = sdn2_::ValueMap;


    using PtrHolder = typename SDN2ArenaBase::PtrHolderT;
    SDN2Document* doc_;
    Array array_;

public:
    SDN2Array(): doc_(), array_() {}

    SDN2Array(SDN2Document* doc, Array array):
        doc_(doc), array_(array)
    {}

    SDN2Array(SDN2Document* doc, PtrHolder ptr):
        doc_(doc), array_(Array::get(&doc_->arena_, ptr))
    {}

    operator SDN2Value() const;

    SDN2Value get(size_t idx) const
    {
        SDN2PtrHolder ptr = array_.access(idx);
        return SDN2Value{doc_, ptr};
    }


    void set(size_t idx, U8StringView value)
    {
        SDN2Ptr<U8MappedString> value_str = doc_->intern(value);
        set_tag(value_str.get(), SDN2ValueTraits<SDN2String>::ValueTag);
        array_.access(idx) = value_str;
    }

    void set(size_t idx, int64_t value)
    {
        SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(SDN2ValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), SDN2ValueTraits<int64_t>::ValueTag);
        array_.access(idx) = value_ptr;
    }

    void set(size_t idx, double value)
    {
        SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(SDN2ValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), SDN2ValueTraits<double>::ValueTag);
        array_.access(idx) = value_ptr;
    }

    void add(U8StringView value)
    {
        SDN2Ptr<U8MappedString> value_str = doc_->intern(value);
        set_tag(value_str.get(), SDN2ValueTraits<SDN2String>::ValueTag);
        array_.push_back(value_str);
    }

    void add(int64_t value)
    {
        SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(SDN2ValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), SDN2ValueTraits<int64_t>::ValueTag);
        array_.push_back(value_ptr);
    }

    void add(double value)
    {
        SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(SDN2ValueTag), &doc_->arena_, value);
        set_tag(value_ptr.get(), SDN2ValueTraits<double>::ValueTag);
        array_.push_back(value_ptr);
    }

    SDN2Map add_map();

    SDN2Array add_array()
    {
        Array value = Array::create_tagged(sizeof(SDN2ValueTag), &doc_->arena_, 4);
        set_tag(value.ptr(), SDN2ValueTraits<SDN2Array>::ValueTag);
        array_.push_back(value.ptr());
        return SDN2Array(doc_, value);
    }

    void remove(size_t idx)
    {
        array_.remove(idx, 1);
    }

    void for_each(std::function<void(SDN2Value)> fn) const
    {
        array_.for_each([&](const auto& value){
            fn(SDN2Value{doc_, value});
        });
    }

    void dump(std::ostream& out, size_t indent = 0) const
    {
        if (size() > 0)
        {
            out << "[\n";

            bool first = true;

            for_each([&](auto vv){
                if (MMA1_LIKELY(!first)) {
                    out << ",\n";
                }
                else {
                    first = false;
                }

                sdn2_::make_indent(out, indent + 1);
                vv.dump(out, indent + 1);
            });
            out << "\n";

            sdn2_::make_indent(out, indent);
            out << "]";
        }
        else {
            out << "[]";
        }
    }

    size_t size() const {
        return array_.size();
    }

private:
    void set_tag(SDN2PtrHolder ptr, SDN2ValueTag tag)
    {
        SDN2Ptr<SDN2ValueTag> tag_ptr(ptr - sizeof(SDN2ValueTag));
        *tag_ptr.get(&doc_->arena_) = tag;
    }

    SDN2ValueTag get_tag(SDN2PtrHolder ptr) const noexcept
    {
        SDN2Ptr<SDN2ValueTag> tag_ptr(ptr - sizeof(SDN2ValueTag));
        return *tag_ptr.get(&doc_->arena_);
    }
};




class SDN2Map {
    using ValueMap = sdn2_::ValueMap;
    using Array = sdn2_::Array;

    using PtrHolder = typename SDN2ArenaBase::PtrHolderT;
    SDN2Document* doc_;
    ValueMap map_;
public:
    SDN2Map(): doc_(), map_() {}

    SDN2Map(SDN2Document* doc, SDN2Ptr<ValueMap::State> map):
        doc_(doc), map_(&doc_->arena_, map)
    {}

    SDN2Map(SDN2Document* doc, ValueMap map):
        doc_(doc), map_(map)
    {}

    operator SDN2Value() const {
        return as_value();
    }

    SDN2Value as_value() const {
        return SDN2Value{doc_, map_.ptr()};
    }

    Optional<SDN2Value> get(U8StringView name) const
    {
        Optional<SDN2PtrHolder> ptr = map_.get(name);
        if (ptr) {
            return SDN2Value(doc_, ptr.get());
        }
        else {
            return Optional<SDN2Value>{};
        }
    }

    void set(U8StringView name, U8StringView value)
    {
        SDN2Ptr<U8MappedString> name_str  = doc_->intern(name);
        SDN2Ptr<U8MappedString> value_str = doc_->intern(value);

        set_tag(value_str.get(), SDN2ValueTraits<SDN2String>::ValueTag);

        map_.put(name_str, value_str);
    }

    void set(U8StringView name, int64_t value)
    {
        SDN2Ptr<U8MappedString> name_str  = doc_->intern(name);

        SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(SDN2ValueTag), &doc_->arena_, value);

        set_tag(value_ptr.get(), SDN2ValueTraits<int64_t>::ValueTag);

        map_.put(name_str, value_ptr);
    }

    void set(U8StringView name, double value)
    {
        SDN2Ptr<U8MappedString> name_str  = doc_->intern(name);

        SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(SDN2ValueTag), &doc_->arena_, value);

        set_tag(value_ptr.get(), SDN2ValueTraits<double>::ValueTag);

        map_.put(name_str, value_ptr);
    }

    void set(SDN2String name, SDN2Value value)
    {
        SDN2Ptr<U8MappedString> name_str  = doc_->intern(name.view());
        map_.put(name_str, value.value_ptr_);
    }

    SDN2Map add_map(U8StringView name)
    {
        SDN2Ptr<U8MappedString> name_str = doc_->intern(name);
        ValueMap value = ValueMap::create(&doc_->arena_, sizeof(SDN2ValueTag));

        set_tag(value.ptr(), SDN2ValueTraits<SDN2Map>::ValueTag);

        map_.put(name_str, value.ptr());
        return SDN2Map(doc_, value);
    }

    SDN2Array add_array(U8StringView name)
    {
        SDN2Ptr<U8MappedString> name_str = doc_->intern(name);
        Array value =  Array::create_tagged(sizeof(SDN2ValueTag), &doc_->arena_, 4);

        set_tag(value.ptr(), SDN2ValueTraits<SDN2Array>::ValueTag);

        map_.put(name_str, value.ptr());
        return SDN2Array(doc_, value);
    }

    void remove(U8StringView name)
    {
        map_.remove(name);
    }

    size_t size() const {
        return map_.size();
    }

    void for_each(std::function<void(U8StringView, SDN2Value)> fn) const
    {
        map_.for_each([&](const auto& key, const auto& value){
            U8StringView kk = key.get(&doc_->arena_)->view();
            fn(kk, SDN2Value{doc_, value});
        });
    }

    void dump(std::ostream& out, size_t indent = 0) const
    {
        if (size() > 0)
        {
            out << "{\n";

            bool first = true;

            for_each([&](auto kk, auto vv){
                if (MMA1_LIKELY(!first)) {
                    out << ",\n";
                }
                else {
                    first = false;
                }

                sdn2_::make_indent(out, indent + 1);
                out << "'" << kk << "': ";
                vv.dump(out, indent + 1);
            });

            out << "\n";

            sdn2_::make_indent(out, indent);
            out << "}";
        }
        else {
            out << "{}";
        }
    }

private:
    void set_tag(SDN2PtrHolder ptr, SDN2ValueTag tag)
    {
        SDN2Ptr<SDN2ValueTag> tag_ptr(ptr - sizeof(SDN2ValueTag));
        *tag_ptr.get(&doc_->arena_) = tag;
    }

    SDN2ValueTag get_tag(SDN2PtrHolder ptr) const noexcept
    {
        SDN2Ptr<SDN2ValueTag> tag_ptr(ptr - sizeof(SDN2ValueTag));
        return *tag_ptr.get(&doc_->arena_);
    }
};



inline SDN2Array SDN2Value::as_array() const noexcept {
    return SDN2Array(doc_, value_ptr_);
}


inline SDN2Map SDN2Value::as_map() const noexcept {
    return SDN2Map(doc_, value_ptr_);
}


inline SDN2Map SDN2Array::add_map()
{
    ValueMap value = ValueMap::create(&doc_->arena_, sizeof(SDN2ValueTag));
    set_tag(value.ptr(), SDN2ValueTraits<SDN2Map>::ValueTag);
    array_.push_back(value.ptr());
    return SDN2Map(doc_, value);
}


inline SDN2Ptr<U8MappedString> SDN2Document::intern(U8StringView string)
{
    StringSet set;

    auto* sst = state();

    if (!sst->strings) {
        set = StringSet::create(&arena_);
        state()->strings = set.ptr();
    }
    else {
        set = StringSet{&arena_, sst->strings};
    }

    Optional<SDN2Ptr<U8MappedString>> str = set.get(string);
    if (str)
    {
        return str.get();
    }
    else {
        SDN2Ptr<U8MappedString> ss = allocate_tagged<U8MappedString>(sizeof(SDN2ValueTag), &arena_, string);
        set.put(ss);
        return ss;
    }
}

inline SDN2String SDN2Document::new_string(U8StringView view)
{
    SDN2Ptr<U8MappedString> ptr = intern(view);
    set_tag(ptr.get(), SDN2ValueTraits<SDN2String>::ValueTag);
    return SDN2String{this, ptr};
}

inline SDN2Identifier SDN2Document::new_identifier(U8StringView view)
{
    SDN2Ptr<U8MappedString> ptr = intern(view);
    set_tag(ptr.get(), SDN2ValueTraits<SDN2Identifier>::ValueTag);
    return SDN2Identifier{this, ptr};
}

inline SDN2Value SDN2Document::new_integer(int64_t value)
{
    SDN2Ptr<int64_t> value_ptr = allocate_tagged<int64_t>(sizeof(SDN2ValueTag), &arena_, value);
    set_tag(value_ptr.get(), SDN2ValueTraits<int64_t>::ValueTag);
    return SDN2Value{this, value_ptr};
}

inline SDN2Value SDN2Document::new_double(double value)
{
    SDN2Ptr<double> value_ptr = allocate_tagged<double>(sizeof(SDN2ValueTag), &arena_, value);
    set_tag(value_ptr.get(), SDN2ValueTraits<double>::ValueTag);
    return SDN2Value{this, value_ptr};
}

inline SDN2Array SDN2Document::new_array(Span<SDN2Value> span)
{
    Array array = Array::create_tagged(sizeof(SDN2ValueTag), &arena_, span.length());
    set_tag(array.ptr(), SDN2ValueTraits<SDN2Array>::ValueTag);

    for (const SDN2Value& vv: span) {
        array.push_back(vv.value_ptr_);
    }

    return SDN2Array(this, array.ptr());
}

inline SDN2Array SDN2Document::new_array()
{
    Array array = Array::create_tagged(sizeof(SDN2ValueTag), &arena_, 4);
    set_tag(array.ptr(), SDN2ValueTraits<SDN2Array>::ValueTag);

    return SDN2Array(this, array.ptr());
}


inline SDN2Map SDN2Document::new_map()
{
    ValueMap value = ValueMap::create(&arena_, sizeof(SDN2ValueTag));

    set_tag(value.ptr(), SDN2ValueTraits<SDN2Map>::ValueTag);

    return SDN2Map(this, value);
}

inline void SDN2Document::set_value(SDN2Value value)
{
    state()->value = value.value_ptr_;
}


inline SDN2Value SDN2Document::value() const {
    return SDN2Value{const_cast<SDN2Document*>(this), state()->value};
}

inline void SDN2Document::set(U8StringView string)
{
    SDN2Ptr<U8MappedString> ss = allocate_tagged<U8MappedString>(sizeof(SDN2ValueTag), &arena_, string);
    set_tag(ss.get(), SDN2ValueTraits<SDN2String>::ValueTag);
    state()->value = ss.get();
}

inline void SDN2Document::set(int64_t value)
{
    SDN2Ptr<int64_t> ss = allocate_tagged<int64_t>(sizeof(SDN2ValueTag), &arena_, value);
    set_tag(ss.get(), SDN2ValueTraits<int64_t>::ValueTag);
    state()->value = ss;
}

inline void SDN2Document::set(double value)
{
    SDN2Ptr<double> ss = allocate_tagged<double>(sizeof(SDN2ValueTag), &arena_, value);
    set_tag(ss.get(), SDN2ValueTraits<double>::ValueTag);
    state()->value = ss;
}

inline SDN2Map SDN2Document::set_map()
{
    ValueMap value = ValueMap::create(&arena_, sizeof(SDN2ValueTag));

    set_tag(value.ptr(), SDN2ValueTraits<SDN2Map>::ValueTag);
    state()->value = value.ptr();

    return SDN2Map(this, value);
}

inline SDN2Array SDN2Document::set_array()
{
    Array value = Array::create_tagged(sizeof(SDN2ValueTag), &arena_, 4);
    set_tag(value.ptr(), SDN2ValueTraits<SDN2Array>::ValueTag);

    state()->value = value.ptr();

    return SDN2Array(this, value);

}

inline std::ostream& SDN2Document::dump(std::ostream& out) const {
    value().dump(out);
    return out;
}


inline void SDN2Value::dump(std::ostream& out, size_t indent) const
{
    if (is_null()) {
        out << "null";
    }
    else if (is_string()) {
        out << "'" << as_string().view() << "'";
    }
    else if (is_integer()) {
        out << as_integer();
    }
    else if (is_double()) {
        out << as_double();
    }
    else if (is_map()) {
        as_map().dump(out, indent);
    }
    else if (is_array()) {
        as_array().dump(out, indent);
    }
    else {
        out << "<unknown type>";
    }
}

inline SDN2String::operator SDN2Value() const {
    return SDN2Value{doc_, string_.get()};
}

inline SDN2Array::operator SDN2Value() const {
    return SDN2Value{doc_, array_.ptr()};
}

}}
