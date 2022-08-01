
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

#include <memoria/core/types.hpp>
#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/datum_base.hpp>

#include <memoria/core/strings/format.hpp>

#include <memoria/core/memory/malloc.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include <typeinfo>

namespace memoria {

class AnyDatum {
    AnyDatumStorage* storage_;

public:
    AnyDatum() noexcept : storage_(nullptr) {}
    AnyDatum(AnyDatumStorage* storage) noexcept :
        storage_(storage)
    {}

    AnyDatum(AnyDatum&& other) noexcept :
        storage_(other.storage_)
    {
        other.storage_ = nullptr;
    }

    AnyDatum(const AnyDatum&) = delete;

    ~AnyDatum() noexcept
    {
        if (storage_) {
            storage_->destroy();
        }
    }

    AnyDatum& operator=(AnyDatum&& other) noexcept
    {
        if (storage_) {
            storage_->destroy();
        }

        storage_ = other.storage_;
        other.storage_ = nullptr;

        return *this;
    }

    bool operator==(const AnyDatum& other) const noexcept {
        return storage_ == other.storage_ ||
                (
                storage_->data_type_info() == other.storage_->data_type_info() &&
                storage_->equals(other.storage_)
                );
    }


    U8String data_type_str() const {
        return storage_->data_type_str();
    }

    const std::type_info& data_type_info () const noexcept {
        return storage_->data_type_info();
    }

    AnyDatumStorage* release()
    {
        AnyDatumStorage* tmp = storage_;
        storage_ = nullptr;
        return tmp;
    }

    const AnyDatumStorage* get() const
    {
        return storage_;
    }

    operator bool() const {
        return storage_ != nullptr;
    }

    U8String to_sdn_string() const
    {
        if (storage_)
        {
            return storage_->to_sdn_string();
        }
        else {
            return "null";
        }
    }
};


inline std::ostream& operator<<(std::ostream& out, const AnyDatum& datum) {
    out << datum.to_sdn_string();
    return out;
}


template <typename DataType, typename SelectorTag>
class DatumStorageBase: public AnyDatumStorage, public DataType {
protected:
    using ViewType = DTTViewType<DataType>;

    using DatumLifeguardSharedImpl = detail::DefaultLifetimeGuardSharedImpl;

    ViewType view_;
    mutable detail::LifetimeGuardShared* lg_shared_;
public:
    DatumStorageBase(ViewType view) noexcept :
        view_(view),
        lg_shared_(nullptr)
    {}

    ~DatumStorageBase() noexcept {
        if (lg_shared_) {
            lg_shared_->invalidate();
        }
    }

    const ViewType& view() const noexcept {return view_;}

    virtual const DataType& data_type() const noexcept {
        return *this;
    }

    virtual const std::type_info& data_type_info() const noexcept
    {
        return typeid(DataType);
    }

    virtual U8String data_type_str() const
    {
        return make_datatype_signature_string<DataType>();
    }

    virtual const char* data() const noexcept {
        return reinterpret_cast<const char*>(&view_);
    }

    virtual bool equals(const AnyDatumStorage* other) const noexcept {
        return view_ == static_cast<const DatumStorageBase*>(other)->view();
    }

    virtual LifetimeGuard lifetime_guard() const noexcept {
        if (!lg_shared_) {
            lg_shared_ = new DatumLifeguardSharedImpl([&]{
                lg_shared_ = nullptr;
            });
        }

        return LifetimeGuard();
    }
};


template <typename DataType, typename SelectorTag, typename ViewType>
U8String datum_to_sdn_string(DataType, SelectorTag, ViewType&& view)
{
    return format_u8("'{}'@{}", view, make_datatype_signature_string<DataType>());
}

template <typename DataType, typename SelectorTag = typename DataTypeTraits<DataType>::DatumStorageSelector>
class DatumFixedSizeStorage: public DatumStorageBase<DataType, SelectorTag> {
    using Base = DatumStorageBase<DataType, SelectorTag>;
    using typename Base::ViewType;

    using Base::view_;

public:
    DatumFixedSizeStorage(ViewType view) noexcept: Base(view) {}

    virtual void destroy() noexcept
    {
        this->~DatumFixedSizeStorage();
        ::free(this);
    }

    static DatumFixedSizeStorage* create(const ViewType& view)
    {
        DatumFixedSizeStorage* storage = allocate_system<DatumFixedSizeStorage>(1).release();

        try {
            return new (storage) DatumFixedSizeStorage(view);
        }
        catch (...) {
            free_system(storage);
            throw;
        }
    }

    virtual U8String to_sdn_string() const
    {
        return datum_to_sdn_string(DataType(), SelectorTag(), view_);
    }
};



template <typename DataType, typename Selector>
class Datum {
    using ViewType = typename DataTypeTraits<DataType>::ViewType;
    using Storage = DatumStorageBase<DataType, typename DataTypeTraits<DataType>::DatumStorageSelector>;

    Storage* storage_;

public:

    Datum() noexcept : storage_(nullptr) {}
    Datum(ViewType view) noexcept:
        storage_(DataTypeTraits<DataType>::DatumStorage::create(view))
    {}

    Datum(Datum&& other) noexcept:
        storage_(other.storage_)
    {
        other.storage_ = nullptr;
    }

    Datum(const Datum&) = delete;

    ~Datum() noexcept
    {
        if (storage_) {
            storage_->destroy();
        }
    }

    Datum& operator=(Datum&& other) noexcept {
        if (storage_) {
            storage_->destroy();
        }

        storage_ = other.storage_;
        other.storage_ = nullptr;

        return *this;
    }

    bool operator==(const Datum& other) const noexcept {
        return storage_ == other.storage_ || storage_->view() == other.storage_->view();
    }

    bool operator==(const ViewType& other) const noexcept {
        return storage_ && storage_->view() == other;
    }

    const DataType& data_type() const noexcept {
        return storage_->data_type();
    }

    U8String data_type_str() const {
        return storage_->data_type_str();
    }

    const std::type_info& data_type_info () const noexcept {
        return storage_->data_type_info();
    }

    const ViewType& view() const noexcept
    {
        return storage_->view();
    }

    GuardedView<ViewType> guarded_view() const noexcept {
        return GuardedView<ViewType>(storage_->view(), storage_->lifetime_guard());
    }

    operator const ViewType& () const noexcept{
        return storage_->view();
    }

    static Datum<DataType, Selector> from_sdn_string(U8StringView sdn_string) {
        return from_sdn(LDDocument::parse(sdn_string));
    }

    static Datum<DataType, Selector> from_sdn(const LDDocument& value);


    operator bool() const {
        return storage_ != nullptr;
    }

    operator AnyDatum () {
        AnyDatum any_datum(storage_);
        storage_ = nullptr;
        return any_datum;
    }

    U8String to_sdn_string() const
    {
        if (storage_)
        {
            return storage_->to_sdn_string();
        }
        else {
            return "null";
        }
    }
};


template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, int64_t value);

template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, double value);

template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, bool value);

template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, const U8StringView& value);


template <typename DataType>
class Datum<DataType, FixedSizeDataTypeTag> {
    using MyType = Datum;

    using ViewType = typename DataTypeTraits<DataType>::ViewType;

    ViewType value_;

public:
    constexpr Datum() noexcept: value_() {}
    Datum(ViewType view) noexcept:
        value_(view)
    {}

    bool operator==(const Datum& other) const noexcept {
        return value_ == other.value_;
    }

    bool operator==(const ViewType& other) const noexcept {
        return value_ == other;
    }

    const DataType& data_type() const noexcept {
        return *this;
    }

    U8String data_type_str() const {
        return make_datatype_signature_string<DataType>();
    }

    const std::type_info& data_type_info () const noexcept {
        return typeid(DataType);
    }

    const ViewType& view() const noexcept
    {
        return value_;
    }

    ViewType& view() noexcept
    {
        return value_;
    }

    const ViewType& guarded_view() const noexcept
    {
        return value_;
    }

    operator const ViewType& () const noexcept {
        return value_;
    }

    static Datum<DataType, FixedSizeDataTypeTag> from_sdn_string(U8StringView sdn_string) {
        return from_sdn(LDDocument::parse(sdn_string));
    }

    static Datum<DataType> from_sdn(const LDDocument& doc)
    {
        auto value = doc.value();

        if (value->is_double()) {
            return datum_from_sdn_value(static_cast<DataType*>(nullptr), (double)*value->as_double());
        }
        else if (value->is_bigint()) {
            return datum_from_sdn_value(static_cast<DataType*>(nullptr), (int64_t)*value->as_bigint());
        }
//        else if (value.is_boolean()) {
//            return datum_from_sdn_value(static_cast<DataType*>(nullptr), value.as_boolean());
//        }

        MMA_THROW(RuntimeException())
                << format_ex(
                       "Unsupported SDN value type for fixed size datum convertion: {}",
                       value->to_standard_string()
                   );
    }



    operator AnyDatum ()
    {
        using StorageTag = typename DataTypeTraits<DataType>::DatumStorageSelector;
        using Storage = DatumFixedSizeStorage<DataType, StorageTag>;

        Storage* storage = Storage::create(value_);        
        return AnyDatum(storage);
    }

    operator bool() const {
        return true;
    }

    U8String to_sdn_string() const
    {
        return datum_to_sdn_string(DataType(), FixedSizeDataTypeTag(), value_);
    }
};




namespace detail {

    template <typename T, typename SelectorTag = typename DataTypeTraits<T>::DatumSelector>
    struct AnyDatumConverter {
        Datum<T, SelectorTag> cast(AnyDatum&& any) {
            return Datum<T, SelectorTag>(any.release());
        }
    };


    template <typename T>
    struct AnyDatumConverter<T, FixedSizeDataTypeTag> {
        using Storage = DatumStorageBase<
            T,
            typename DataTypeTraits<T>::DatumStorageSelector
        >;

        static Datum<T, FixedSizeDataTypeTag> cast(AnyDatum&& any)
        {
            const Storage* datum = static_cast<const Storage*>(any.get());
            return Datum<T, FixedSizeDataTypeTag>(datum->view());
        }
    };
}

template <typename T>
Datum<T> datum_cast(AnyDatum&& any)
{
    return detail::AnyDatumConverter<T>::cast(std::move(any));
}


}
