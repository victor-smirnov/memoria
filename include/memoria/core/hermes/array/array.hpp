
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

#include <memoria/core/arena/array.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/hermes/array/generic_array.hpp>

#include <memoria/core/hermes/array/typed_array.hpp>
#include <memoria/core/hermes/array/typed_array_1fse.hpp>

#include <memoria/core/hermes/object_ptr.hpp>

namespace memoria {
namespace hermes {

template <typename V>
class ArrayView;

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
class ArrayView<Object>: public HoldingView<ArrayView<ObjectView>> {
    using Base = HoldingView<ArrayView<ObjectView>>;
public:    
    using ArrayStorageT = arena::Array<arena::ERelativePtr>;
protected:
    static_assert(std::is_standard_layout_v<ArrayStorageT>, "");

    mutable ArrayStorageT* array_;
    using Base::mem_holder_;

    friend class HermesCtrImpl;
    friend class HermesCtr;
    friend class ObjectView;

    template <typename, typename>
    friend class MapView;

    template <typename>
    friend class ArrayView;

    friend class DatatypeView;

    template <typename>
    friend class TypedGenericArray;

    friend class memoria::hermes::path::interpreter::Interpreter;
    friend class HermesCtrBuilder;

    using Accessor = ArrayAccessor<ObjectArray, Object>;
public:
    using iterator = RandomAccessIterator<Accessor>;
    using const_iterator = iterator;

    ArrayView() noexcept:
        array_()
    {}

    ArrayView(LWMemHolder* ref_holder, void* array) noexcept:
        Base(ref_holder),
        array_(reinterpret_cast<ArrayStorageT*>(array))
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

    ObjectArray self() const {
        return ObjectArray(ObjectArrayView(mem_holder_, array_));
    }

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(
                    mem_holder_->ctr(),
                    mem_holder_->owner(),
                    pool::DoRef{}
        );
    }

    Object as_object() const {
        return Object(ObjectView(mem_holder_, array_));
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

    Object get(uint64_t idx) const;

    template <typename DT>
    ObjectArray append(DTTViewType<DT> view);
    Object set_hermes(uint64_t idx, U8StringView str);

    ObjectArray append(const Object& value);
    void set(uint64_t idx, const Object& value);


    template <typename DT>
    DataObject<DT> set(uint64_t idx, DTTViewType<DT> view);
    void set_null(uint64_t idx);

    ObjectArray remove(uint64_t idx);

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

    void for_each(std::function<void(const Object&)> fn) const;

    bool is_null() const {
        return array_ == nullptr;
    }

    bool is_not_null() const {
        return array_ != nullptr;
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return array_->deep_copy_to(arena, ShortTypeCode::of<ArrayView>(), mem_holder_, dedup);
    }

    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), array_, MoveOwnershipTag{});
    }

private:
    void assert_not_null() const {
        if (MMA_UNLIKELY(array_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("ArrayView<Object> is null").do_throw();
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
    LWMemHolder* ctr_holder_;
    mutable ArrayView<Object> array_;
public:
    TypedGenericArray(LWMemHolder* ctr_holder, void* array):
        ctr_holder_(ctr_holder),
        array_(ctr_holder, array)
    {
        ctr_holder->ref_copy();
    }

    virtual ~TypedGenericArray() noexcept {
        ctr_holder_->unref();
    }

    virtual uint64_t size() const {
        return array_.size();
    }

    virtual Object get(uint64_t idx) const {
        return array_.get(idx);
    }

    virtual void set(uint64_t idx, const Object& value) {
        array_.set(idx, value);
    }

    virtual GenericArrayPtr push_back(const Object& value)
    {
        auto new_array = array_.append(value);
        if (MMA_LIKELY(new_array->array_ == array_.array_)) {
            return this->shared_from_this();
        }
        else {
            return make_wrapper(ctr_holder_, new_array->array_);
        }
    }

    virtual GenericArrayPtr remove(uint64_t start, uint64_t end) {
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
        MEMORIA_MAKE_GENERIC_ERROR("Can't convert ArrayView<Object> to GenericMap").do_throw();
    }

    virtual Object as_object() const {
        return array_.as_object();
    }

    static PoolSharedPtr<GenericArray> make_wrapper(LWMemHolder* ctr_holder, void* array);
};


}}
