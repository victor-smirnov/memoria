
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



#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/common.hpp>

#include <memoria/core/hermes/array/generic_array.hpp>

#include <memoria/core/hermes/array/typed_array_1fse.hpp>

namespace memoria {
namespace hermes {

template <typename DT>
class ArrayView: public HoldingView<ArrayView<DT>> {
    using Base = HoldingView<ArrayView<DT>>;
public:
    using ArrayStorageT = detail::TypedArrayData<DT, detail::FSE1Subtype, true>;
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
    friend class HermesCtrBuilder;

    template <typename>
    friend class TypedGenericArray;

    using Accessor = ArrayAccessor<Array<DT>, Object>;
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
        return MemHolderHandle(this->get_mem_holder());
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

    Array<DT> self() const {
        return Array<DT>(mem_holder_, array_);
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

    DTView<DT> get(uint64_t idx) const
    {
        assert_not_null();

        if (idx < array_->size()) {
            return DTView<DT>(mem_holder_, array_->get(idx));
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Range check in Array<DT>: {} {}", idx, array_->size()).do_throw();
        }
    }

    void push_back(DTTViewType<DT> view);

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
        if (is_not_null()) {
            return size() <= 3;
        }

        return false;
    }

    void for_each(std::function<void(const DTView<DT>&)> fn) const {
        assert_not_null();

        for (auto& vv: array_->span()) {
            fn(DTView<DT>(mem_holder_, vv));
        }
    }

    bool is_null() const {
        return array_ == nullptr;
    }

    bool is_not_null() const {
        return array_ != nullptr;
    }

    void* deep_copy_to(DeepCopyState& dedup) const {
        assert_not_null();
        return array_->deep_copy_to(ShortTypeCode::of<Array<DT>>(), dedup);
    }

    void remove(uint64_t element);

    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), array_, MoveOwnershipTag{});
    }

    bool operator!=(const ArrayView<DT>& other) const {
        return !operator==(other);
    }

    bool operator==(const ArrayView<DT>& other) const
    {
        if (is_not_null() && other.is_not_null())
        {
            uint64_t my_size = size();
            if (my_size == other.size())
            {
                for (uint64_t c = 0; c < my_size; c++) {
                    if (!(get(c) == other.get(c))) {
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
        array->check_typed_array(state);
    }

private:
    void assert_not_null() const {
        if (MMA_UNLIKELY(array_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("ArrayView<DT> is null").do_throw();
        }
    }

    void assert_mutable();

    void do_stringify(std::ostream& out, DumpFormatState& state) const;

};


template <typename DT>
class TypedGenericArray: public GenericArray, public pool::enable_shared_from_this<TypedGenericArray<DT>> {    
    mutable Array<DT> array_;
public:
    TypedGenericArray(Array<DT>&& array):
        array_(std::move(array))
    {}

    virtual uint64_t capacity() const {
        return array_.capacity();
    }

    virtual uint64_t size() const {
        return array_.size();
    }

    virtual Object get(uint64_t idx) const {
        return Object(array_.mem_holder(), array_.get(idx), DirectViewTag<DT>{});
    }

    virtual void set(uint64_t idx, const Object& value) {
        array_.set(idx, value.convert_to<DT>().template as_data_object<DT>());
    }

    virtual void push_back(const Object& value) {
        array_.push_back(value.convert_to<DT>().template as_data_object<DT>());
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
        MEMORIA_MAKE_GENERIC_ERROR("Can't convert ArrayView<DT> to GenericMap").do_throw();
    }

    virtual Object as_object() const {
        return array_.as_object();
    }

    static PoolSharedPtr<GenericArray> make_wrapper(Array<DT>&&);
};


}}
