
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

#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/common.hpp>
#include <memoria/core/hermes/traits.hpp>

#include <memoria/core/arena/string.hpp>



namespace memoria {
namespace hermes {

class HermesCtr;

struct IParameterResolver {
    virtual ~IParameterResolver() noexcept = default;

    virtual bool has_parameter(U8StringView name) const = 0;
    virtual ObjectPtr resolve(U8StringView name) const   = 0;
};

struct Params: IParameterResolver {
    virtual bool has_parameter(U8StringView name) const;
    virtual ObjectPtr resolve(U8StringView name) const;

    template <typename DT>
    void add_dataobject(U8StringView name, DTTViewType<DT> view);
    void add_hermes(U8StringView name, U8StringView value);

private:
    std::unordered_map<U8String, ObjectPtr> params_;
};


class Parameter: public HoldingView<Parameter> {
    using Base = HoldingView<Parameter>;
public:
    using ArenaDTContainer = arena::ArenaDataTypeContainer<Varchar>;

    friend class HermesCtr;
    friend class Object;

    template <typename, typename>
    friend class Map;

    template <typename>
    friend class Array;

    using ViewT = DTTViewType<Varchar>;
    using ViewPtrT = ViewPtr<ViewT>;

protected:
    mutable ArenaDTContainer* dt_ctr_;
    using Base::ptr_holder_;
public:
    Parameter() noexcept:
        dt_ctr_()
    {}

    Parameter(ViewPtrHolder* ptr_holder, void* dt_ctr) noexcept :
        Base(ptr_holder),
        dt_ctr_(reinterpret_cast<ArenaDTContainer*>(dt_ctr))
    {}

    PoolSharedPtr<HermesCtr> document() const {
        assert_not_null();
        return PoolSharedPtr<HermesCtr>(
                    ptr_holder_->ctr(),
                    ptr_holder_->owner(),
                    pool::DoRef{}
        );
    }


    U8String to_plain_string() const
    {
        return *view();
    }

    uint64_t hash_code() const;

    bool is_null() const noexcept {
        return dt_ctr_ == nullptr;
    }

    bool is_not_null() const noexcept {
        return dt_ctr_ != nullptr;
    }

    ObjectPtr as_object() const {
        return ObjectPtr(Object(ptr_holder_, dt_ctr_));
    }

    ViewPtrT view() const
    {
        assert_not_null();
        return wrap(dt_ctr_->view());
    }



    U8String to_string(const StringifyCfg& cfg = StringifyCfg()) const
    {
        DumpFormatState fmt = DumpFormatState(cfg);
        std::stringstream ss;
        stringify(ss, fmt);
        return ss.str();
    }

    U8String to_pretty_string() const
    {
        return to_string(StringifyCfg::pretty());
    }

    void stringify(std::ostream& out, const StringifyCfg& cfg) const
    {
        DumpFormatState state(cfg);
        stringify(out, state);
    }


    void stringify(std::ostream& out,
                   DumpFormatState& state) const
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
        return dt_ctr_->deep_copy_to(arena, ShortTypeCode::of<Parameter>(), ptr_holder_, dedup);
    }

    int32_t compare(const ParameterPtr& other) const
    {
        if (is_not_null() && other->is_not_null())
        {
            return view()->compare(*other->view());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Comparing operands may not be nullptr").do_throw();
        }
    }


    bool equals(const ParameterPtr& other) const
    {
        if (is_not_null() && other->is_not_null()) {
            return *view() == *other->view();
        }
        else {
            return false;
        }
    }

private:
    ViewPtrT wrap(const ViewT& view) const {
        return ViewPtrT(view, this->get_ptr_holder());
    }

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


}}
