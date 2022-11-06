
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

#include <memoria/core/hermes/traits.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/hermes/array/typed_array_1fse.hpp>

namespace memoria {
namespace hermes {

template <typename DT>
class Array: public HoldingView<Array<DT>> {
    using Base = HoldingView<Array<DT>>;
public:
    using ArrayStorageT = detail::TypedArrayData<DT, detail::FSE1Subtype, true>;
protected:
    mutable ArrayStorageT* array_;
    mutable HermesCtr* doc_;
    using Base::ptr_holder_;

    friend class HermesCtrImpl;
    friend class HermesCtr;
    friend class Value;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    friend class Datatype;

    friend class memoria::hermes::path::interpreter::Interpreter;
    friend class HermesCtrBuilder;

//    using Accessor = ArrayAccessor<GenericArrayPtr, ValuePtr>;
public:
//    using iterator = RandomAccessIterator<Accessor>;
//    using const_iterator = iterator;


    Array() noexcept:
        array_(), doc_()
    {}

    Array(void* array, HermesCtr* doc, ViewPtrHolder* ref_holder) noexcept:
        Base(ref_holder),
        array_(reinterpret_cast<ArrayStorageT*>(array)), doc_(doc)
    {}

//    iterator begin() {
//        assert_not_null();
//        return iterator(Accessor{self()}, 0, array_->size());
//    }

//    iterator end() {
//        assert_not_null();
//        return iterator(Accessor{self()}, array_->size(), array_->size());
//    }

//    iterator cbegin() const {
//        assert_not_null();
//        return const_iterator(Accessor{self()}, array_->size(), array_->size());
//    }

//    iterator cend() const {
//        assert_not_null();
//        return const_iterator(Accessor{self()}, array_->size(), array_->size());
//    }

    ArrayPtr<DT> self() const {
        return ArrayPtr<DT>(Array<DT>(array_, doc_, ptr_holder_));
    }

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }

//    ValuePtr as_value() const {
//        return ValuePtr(Value(array_, doc_, ptr_holder_));
//    }

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

//    ValuePtr get(uint64_t idx) const
//    {
//        assert_not_null();

//        if (idx < array_->size())
//        {
//            if (MMA_LIKELY(array_->get(idx).is_not_null())) {
//                return ValuePtr(Value(array_->get(idx).get(), doc_, ptr_holder_));
//            }
//            else {
//                return ValuePtr{};
//            }
//        }
//        else {
//            MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<Value>: {} {}", idx, array_->size()).do_throw();
//        }
//    }


    void append(DTTViewType<DT> view);

    void set(uint64_t idx, DTTViewType<DT> view);


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

//    void for_each(std::function<void(const ViewPtr<Value>&)> fn) const {
//        assert_not_null();

//        for (auto& vv: array_->span()) {
//            fn(ViewPtr<Value>(Value(vv.get(), doc_, ptr_holder_)));
//        }
//    }

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

private:
    void assert_not_null() const {
        if (MMA_UNLIKELY(array_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Array<Value> is null").do_throw();
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

}}
