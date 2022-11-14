
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

#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/hermes/array/generic_array.hpp>

#include <memoria/core/hermes/array/typed_array.hpp>
#include <memoria/core/hermes/array/typed_array_1fse.hpp>


namespace memoria {
namespace hermes {

template <typename V>
class Array;

template <typename ArrayT, typename ValueT>
struct ArrayAccessor {
    using ViewType = ValueT;

    mutable ArrayT array;

    ValueT get(uint64_t idx) const {
        return array->get(idx);
    }

    bool operator==(const ArrayAccessor& other) const {
        return array->equals(other.array);
    }
};

template <>
class Array<Object>: public HoldingView<Array<Object>> {
    using Base = HoldingView<Array<Object>>;
public:    
    using ArrayStorageT = arena::Vector<arena::RelativePtr<void>>;
protected:
    static_assert(std::is_standard_layout_v<ArrayStorageT>, "");

    mutable ArrayStorageT* array_;
    mutable HermesCtr* doc_;

    using Base::ptr_holder_;

    friend class HermesCtrImpl;
    friend class HermesCtr;
    friend class Object;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    friend class Datatype;

    template <typename>
    friend class TypedGenericArray;

    friend class memoria::hermes::path::interpreter::Interpreter;
    friend class HermesCtrBuilder;

    using Accessor = ArrayAccessor<ObjectArrayPtr, ObjectPtr>;
public:
    using iterator = RandomAccessIterator<Accessor>;
    using const_iterator = iterator;

    Array() noexcept:
        array_(), doc_()
    {}

    Array(void* array, HermesCtr* doc, ViewPtrHolder* ref_holder) noexcept:
        Base(ref_holder),
        array_(reinterpret_cast<ArrayStorageT*>(array)), doc_(doc)
    {}

    iterator begin() {
        assert_not_null();
        return iterator(Accessor{self()}, 0, array_->size());
    }

    iterator end() {
        assert_not_null();
        return iterator(Accessor{self()}, array_->size(), array_->size());
    }

    iterator cbegin() const {
        assert_not_null();
        return const_iterator(Accessor{self()}, array_->size(), array_->size());
    }

    iterator cend() const {
        assert_not_null();
        return const_iterator(Accessor{self()}, array_->size(), array_->size());
    }

    PoolSharedPtr<GenericArray> as_generic_array() const;

    ObjectArrayPtr self() const {
        return ObjectArrayPtr(ObjectArray(array_, doc_, ptr_holder_));
    }

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

    ObjectPtr as_object() const {
        return ObjectPtr(Object(array_, doc_, ptr_holder_));
    }

    uint64_t size() const {
        assert_not_null();
        return array_->size();
    }


    bool empty() const {
        return size() == 0;
    }

    bool is_empty() const {
        assert_not_null();
        return array_->size() == 0;
    }

    ObjectPtr get(uint64_t idx) const
    {
        assert_not_null();

        if (idx < array_->size())
        {
            if (MMA_LIKELY(array_->get(idx).is_not_null())) {
                return ObjectPtr(Object(array_->get(idx).get(), doc_, ptr_holder_));
            }
            else {
                return ObjectPtr{};
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Object>: {} {}", idx, array_->size()).do_throw();
        }
    }

    template <typename DT>
    DataObjectPtr<DT> append(DTTViewType<DT> view);

    ObjectPtr append_hermes(U8StringView str);
    ObjectPtr set_hermes(uint64_t idx, U8StringView str);

    ObjectPtr append_null();

    template <typename DT>
    DataObjectPtr<DT> set(uint64_t idx, DTTViewType<DT> view);
    void set_null(uint64_t idx);

    void stringify(std::ostream& out,
                   DumpFormatState& state) const
    {
        assert_not_null();

        if (state.cfg().spec().indent_size() == 0 || !is_simple_layout()) {
            do_stringify(out, state);
        }
        else {
            StringifyCfg cfg1 = state.cfg();
            cfg1.with_spec(StringifySpec::simple());
            DumpFormatState simple_state(cfg1);
            do_stringify(out, simple_state);
        }
    }

    bool is_simple_layout() const
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

    void for_each(std::function<void(const ViewPtr<Object>&)> fn) const {
        assert_not_null();

        for (auto& vv: array_->span()) {
            if (vv.is_not_null()) {
                fn(ViewPtr<Object>(Object(vv.get(), doc_, ptr_holder_)));
            }
            else {
                fn(ViewPtr<Object>(Object()));
            }
        }
    }

    bool is_null() const {
        return array_ == nullptr;
    }

    bool is_not_null() const {
        return array_ != nullptr;
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return array_->deep_copy_to(arena, ShortTypeCode::of<Array>(), doc_, ptr_holder_, dedup);
    }

    void remove(uint64_t idx);


    void append(const ObjectPtr& value);
    void set(uint64_t idx, const ObjectPtr& value);

private:
    void assert_not_null() const {
        if (MMA_UNLIKELY(array_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Array<Object> is null").do_throw();
        }
    }

    void assert_mutable();

    void do_stringify(std::ostream& out, DumpFormatState& state) const
    {
        auto& spec = state.cfg().spec();

        if (size() > 0)
        {
            out << "[" << spec.nl_start();

            bool first = true;

            state.push();
            for_each([&](auto vv){
                if (MMA_LIKELY(!first)) {
                    out << "," << spec.nl_middle();
                }
                else {
                    first = false;
                }

                state.make_indent(out);
                vv->stringify(out, state);
            });
            state.pop();

            out << spec.nl_end();

            state.make_indent(out);
            out << "]";
        }
        else {
            out << "[]";
        }
    }
};

template<>
class TypedGenericArray<Object>: public GenericArray, public pool::enable_shared_from_this<TypedGenericArray<Object>> {
    ViewPtrHolder* ctr_holder_;
    mutable Array<Object> array_;
public:
    TypedGenericArray(void* array, HermesCtr* ctr, ViewPtrHolder* ctr_holder):
        ctr_holder_(ctr_holder),
        array_(array, ctr, ctr_holder)
    {
        ctr_holder->ref_copy();
    }

    virtual ~TypedGenericArray() noexcept {
        ctr_holder_->unref();
    }

    virtual uint64_t size() const {
        return array_.size();
    }

    virtual ObjectPtr get(uint64_t idx) const {
        return array_.get(idx);
    }

    virtual void set(uint64_t idx, const ObjectPtr& value) {
        array_.set(idx, value);
    }

    virtual void push_back(const ObjectPtr& value) {
        array_.append(value);
    }

    virtual void remove(uint64_t start, uint64_t end) {
        MEMORIA_MAKE_GENERIC_ERROR("Not implemented").do_throw();
    }

    virtual PoolSharedPtr<HermesCtr> ctr() const {
        return array_.document();
    }

    virtual bool is_array() const {
        return true;
    }

    virtual bool is_map() const {
        return false;
    }


    virtual bool empty() const {
        return array_.size() == 0;
    }

    virtual PoolSharedPtr<GenericArray> as_array() const {
        return this->shared_from_this();
    }

    virtual PoolSharedPtr<GenericMap> as_map() const {
        MEMORIA_MAKE_GENERIC_ERROR("Can't convert Array<Object> to GenericMap").do_throw();
    }

    virtual ObjectPtr as_object() const {
        return array_.as_object();
    }

    static PoolSharedPtr<GenericArray> make_wrapper(void* array, HermesCtr* ctr, ViewPtrHolder* ctr_holder);
};


}}
