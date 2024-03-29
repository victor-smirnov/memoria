
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
class ArrayView;



template <>
class ArrayView<Object>: public HoldingView<ArrayView<ObjectView>> {
    using Base = HoldingView<ArrayView<ObjectView>>;
public:    
    using ArrayStorageT = arena::Vector<arena::ERelativePtr>;
protected:
    static_assert(std::is_standard_layout_v<ArrayStorageT>, "");

    mutable ArrayStorageT* array_;
    using Base::mem_holder_;

    friend class HermesCtrImpl;
    friend class HermesCtrView;
    friend class ObjectView;

    template <typename, typename>
    friend class MapView;

    template <typename>
    friend class ArrayView;

    friend class DatatypeView;

    template <typename>
    friend class TypedGenericArray;

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

    MemHolderHandle mem_holder() const {
        assert_not_null();
        return MemHolderHandle(get_mem_holder());
    }

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

    HermesCtr ctr() const;

    Object as_object() const {
        return Object(mem_holder_, array_);
    }

    uint64_t size() const {
        assert_not_null();
        return array_->size();
    }


    uint64_t capacity() const {
        assert_not_null();
        return array_->capacity();
    }


    bool empty() const {
        return size() == 0;
    }

    bool is_empty() const {
        assert_not_null();
        return array_->size() == 0;
    }

    MaybeObject get(uint64_t idx) const;

    template <typename T, std::enable_if_t<!HermesObject<std::decay_t<T>>::Value, int> = 0>
    Object set(uint64_t idx, T&& value);

    template <typename DT, typename T>
    Object set_t(uint64_t idx, T&& value);

    MaybeObject set(uint64_t idx, const MaybeObject& value);

    template <typename T, std::enable_if_t<!HermesObject<std::decay_t<T>>::Value, int> = 0>
    void push_back(T&& view);

    template <typename DT, typename T>
    void push_back_t(T&& view);
    void push_back(const MaybeObject& value);

    void set_null(uint64_t idx);

    void remove(uint64_t idx);

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
            simple = simple && vv ? vv->is_simple_layout() : true;
        });

        return simple;
    }

    void for_each(std::function<void(const MaybeObject&)> fn) const;

    bool is_null() const {
        return array_ == nullptr;
    }

    bool is_not_null() const {
        return array_ != nullptr;
    }

    void* deep_copy_to(DeepCopyState& dedup) const {
        assert_not_null();
        return array_->deep_copy_to(ShortTypeCode::of<Array<Object>>(), dedup);
    }

    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), array_, MoveOwnershipTag{});
    }

    bool operator!=(const ArrayView<Object>& other) const {
        return !operator==(other);
    }

    bool operator==(const ArrayView<Object>& other) const
    {
        if (is_not_null() && other.is_not_null())
        {
            uint64_t my_size = size();
            if (my_size == other.size())
            {
                for (uint64_t c = 0; c < my_size; c++) {
                    if (get(c) != other.get(c)) {
                        return false;
                    }
                }

                return true;
            }
            else {
                return false;
            }
        }

        return is_null() && other.is_null();
    }

    static void check_structure(const void* addr, CheckStructureState& state)
    {
        state.check_alignment<ArrayStorageT>(addr, MA_SRC);

        const ArrayStorageT* array
                = reinterpret_cast<const ArrayStorageT*>(addr);
        array->check_object_array(state);
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
                if (vv) {
                    vv->stringify(out, state);
                }
                else {
                    out << "null";
                }
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
    mutable Array<Object> array_;
public:
    TypedGenericArray(Array<Object>&& array):
        array_(std::move(array))
    {}

    virtual uint64_t size() const {
        return array_.size();
    }

    virtual uint64_t capacity() const {
        return array_.capacity();
    }

    virtual Object get(uint64_t idx) const {
        return array_.get(idx);
    }

    virtual void set(uint64_t idx, const Object& value) {
        array_.set(idx, value);
    }

    virtual void push_back(const Object& value)
    {
        array_.push_back(value);
    }

    virtual void remove(uint64_t start, uint64_t end) {
        MEMORIA_MAKE_GENERIC_ERROR("Not implemented").do_throw();
    }

    virtual HermesCtr ctr() const;

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

    static PoolSharedPtr<GenericArray> make_wrapper(Array<Object>&&);
};


}}
