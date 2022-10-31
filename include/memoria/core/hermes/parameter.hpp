
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/arena/string.hpp>
#include <memoria/core/arena/hash_fn.hpp>

#include <memoria/core/hermes/value.hpp>
#include <memoria/core/hermes/common.hpp>
#include <memoria/core/hermes/traits.hpp>

#include <memoria/core/arena/string.hpp>



namespace memoria {
namespace hermes {

class HermesCtr;

struct IParameterResolver {
    virtual ~IParameterResolver() noexcept = default;

    virtual bool has_parameter(U8StringView name) const = 0;
    virtual ValuePtr resolve(U8StringView name) const   = 0;
};

struct Params: IParameterResolver {
    virtual bool has_parameter(U8StringView name) const;
    virtual ValuePtr resolve(U8StringView name) const;

    template <typename DT>
    void add_dataobject(U8StringView name, DTTViewType<DT> view);
    void add_hermes(U8StringView name, U8StringView value);

private:
    std::unordered_map<U8String, ValuePtr> params_;
};


class Parameter: public HoldingView {
public:
    using ArenaDTContainer = arena::ArenaDataTypeContainer<Varchar>;

    friend class HermesCtr;
    friend class Value;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

protected:
    mutable ArenaDTContainer* dt_ctr_;
    mutable HermesCtr* doc_;
public:
    Parameter() noexcept:
        dt_ctr_(), doc_()
    {}

    Parameter(void* dt_ctr, HermesCtr* doc, ViewPtrHolder* ptr_holder) noexcept :
        HoldingView(ptr_holder),
        dt_ctr_(reinterpret_cast<ArenaDTContainer*>(dt_ctr)),
        doc_(doc)
    {}

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(doc_, ptr_holder_->owner(), pool::DoRef{});
    }


    U8String to_plain_string() const
    {
        return view();
    }

    uint64_t hash_code() const;

    bool is_null() const noexcept {
        return dt_ctr_ == nullptr;
    }

    bool is_not_null() const noexcept {
        return dt_ctr_ != nullptr;
    }

    ValuePtr as_value() const {
        return ValuePtr(Value(dt_ctr_, doc_, ptr_holder_));
    }

    U8StringView view() const
    {
        assert_not_null();
        return dt_ctr_->view();
    }

    U8String to_string() const
    {
        DumpFormatState fmt = DumpFormatState().simple();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        DumpFormatState fmt = DumpFormatState();
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    void stringify(std::ostream& out) const
    {
        DumpFormatState state;
        DumpState dump_state(*doc_);
        stringify(out, state, dump_state);
    }

    void stringify(std::ostream& out, DumpFormatState& format) const
    {
        DumpState dump_state(*doc_);
        stringify(out, format, dump_state);
    }



    void stringify(std::ostream& out,
                   DumpFormatState& state,
                   DumpState& dump_state) const
    {
        if (dt_ctr_) {
            out << "?" << dt_ctr_->view();
        }
        else {
            out << "<null parameter>";
        }
    }

    bool is_simple_layout() const {
        return true;
    }

    void* deep_copy_to(arena::ArenaAllocator& arena, DeepCopyDeduplicator& dedup) const {
        assert_not_null();
        return dt_ctr_->deep_copy_to(arena, TypeHashV<Parameter>, doc_, ptr_holder_, dedup);
    }

    int32_t compare(const ParameterPtr& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            return view().compare(other->view());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Comparing operands may not be nullptr").do_throw();
        }
    }


    bool equals(const ParameterPtr& other) const
    {
        if (is_not_null() && other->is_not_null()) {
            return view() == other->view();
        }
        else {
            return false;
        }
    }

private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(dt_ctr_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("Parameter is null").do_throw();
        }
    }
};


static inline std::ostream& operator<<(std::ostream& out, const ParameterPtr& ptr) {
    out << ptr->to_string();
    return out;
}

namespace detail {

template <>
struct ValueCastHelper<Parameter> {
    static ParameterPtr cast_to(void* addr, HermesCtr* doc, ViewPtrHolder* ref_holder) noexcept {
        return ParameterPtr(Parameter(
            addr,
            doc,
            ref_holder
        ));
    }
};

}


}}
