
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
class Array<Value>: public HoldingView {
public:
    using ArenaArray = arena::Vector<arena::RelativePtr<void>>;
protected:
    mutable ArenaArray* array_;
    mutable HermesDocView* doc_;

    friend class HermesDocImpl;
    friend class HermesDocView;
    friend class Value;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    friend class Datatype;

    friend class memoria::jmespath::interpreter::Interpreter;

    using Accessor = ArrayAccessor<GenericArrayPtr, ValuePtr>;
public:
    using iterator = RandomAccessIterator<Accessor>;
    using const_iterator = iterator;


    Array() noexcept:
        array_(), doc_()
    {}

    Array(void* array, HermesDocView* doc, ViewPtrHolder* ref_holder) noexcept:
        HoldingView(ref_holder),
        array_(reinterpret_cast<ArenaArray*>(array)), doc_(doc)
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

    GenericArrayPtr self() const {
        return GenericArrayPtr(GenericArray(array_, doc_, ptr_holder_));
    }

    PoolSharedPtr<HermesDocView> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesDocView>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

    ValuePtr as_value() const {
        return ValuePtr(Value(array_, doc_, ptr_holder_));
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

    ValuePtr get(uint64_t idx) const
    {
        assert_not_null();

        if (idx < array_->size())
        {
            if (MMA_LIKELY(array_->get(idx).is_not_null())) {
                return ValuePtr(Value(array_->get(idx).get(), doc_, ptr_holder_));
            }
            else {
                return ValuePtr{};
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>: {} {}", idx, array_->size()).do_throw();
        }
    }

    template <typename DT>
    DataObjectPtr<DT> append(DTTViewType<DT> view);

    GenericMapPtr append_generic_map();
    GenericArrayPtr append_generic_array();

    DatatypePtr append_datatype(U8StringView name);
    DatatypePtr append_datatype(const StringValuePtr& name);

    ValuePtr append_hermes(U8StringView str);
    ValuePtr set_hermes(uint64_t idx, U8StringView str);

    ValuePtr append_null();

    template <typename DT>
    DataObjectPtr<DT> set(uint64_t idx, DTTViewType<DT> view);
    void set_null(uint64_t idx);

    GenericMapPtr set_generic_map(uint64_t idx);
    GenericArrayPtr set_generic_array(uint64_t idx);

    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state) const
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

    void for_each(std::function<void(const ViewPtr<Value>&)> fn) const {
        assert_not_null();

        for (auto& vv: array_->span()) {
            fn(ViewPtr<Value>(Value(vv.get(), doc_, ptr_holder_)));
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
        return array_->deep_copy_to(arena, TypeHashV<Array>, doc_, ptr_holder_, dedup);
    }

    void remove(uint64_t idx);


    bool equals(const Array& vv) const noexcept {
        return array_ == vv.array_;
    }

    bool equals(const GenericArrayPtr& vv) const noexcept {
        return array_ == vv->array_;
    }


protected:
    void append(const ValuePtr& value);
    void set_value(uint64_t idx, const ValuePtr& value);

private:
    void assert_not_null() const {
        if (MMA_UNLIKELY(array_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Array<Value> is null").do_throw();
        }
    }

    void assert_mutable();

    void do_stringify(std::ostream& out, DumpFormatState& state, DumpState& dump_state) const
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


namespace detail {

template <>
struct ValueCastHelper<GenericArray> {
    static GenericArrayPtr cast_to(void* addr, HermesDocView* doc, ViewPtrHolder* ref_holder) noexcept {
        return GenericArrayPtr(GenericArray(
            addr,
            doc,
            ref_holder
        ));
    }
};

}


}}
