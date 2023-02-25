
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

class HermesCtrView;

struct IParameterResolver {
    virtual ~IParameterResolver() noexcept = default;

    virtual bool has_parameter(U8StringView name) const = 0;
    virtual Object resolve(U8StringView name) const   = 0;
};

struct Params: IParameterResolver {
    virtual bool has_parameter(U8StringView name) const;
    virtual Object resolve(U8StringView name) const;

    template <typename DT>
    void add_dataobject(U8StringView name, DTTViewType<DT> view);
    void add_hermes(U8StringView name, U8StringView value);

private:
    std::unordered_map<U8String, Object> params_;
};


class ParameterView: public HoldingView<ParameterView> {
    using Base = HoldingView<ParameterView>;
public:
    using ArenaDTContainer = arena::ArenaDataTypeContainer<Varchar>;

    friend class HermesCtrView;
    friend class ObjectView;

    template <typename, typename>
    friend class MapView;

    template <typename>
    friend class ArrayView;

    using ViewT = DTTViewType<Varchar>;
    using ViewPtrT = DTView<Varchar>;

protected:
    mutable ArenaDTContainer* dt_ctr_;
    using Base::mem_holder_;
public:
    ParameterView() noexcept:
        dt_ctr_()
    {}

    ParameterView(LWMemHolder* ptr_holder, void* dt_ctr) noexcept :
        Base(ptr_holder),
        dt_ctr_(reinterpret_cast<ArenaDTContainer*>(dt_ctr))
    {}

    ParameterView(MemHolderHandle&& holder, void* dt_ctr) noexcept :
        ParameterView(holder.release(), dt_ctr)
    {}

    MemHolderHandle mem_holder() const {
        assert_not_null();
        return MemHolderHandle(this->get_mem_holder());
    }

    HermesCtr ctr() const;


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

    Object as_object() const {
        return Object(ObjectView(mem_holder_, dt_ctr_));
    }

    ViewPtrT view() const
    {
        assert_not_null();
        return dt_ctr_->view(mem_holder_);
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
            out << "?" << dt_ctr_->view(mem_holder_).to_string();
        }
        else {
            out << "<null parameter>";
        }
    }

    bool is_simple_layout() const {
        return true;
    }

    void* deep_copy_to(DeepCopyState& dedup) const {
        assert_not_null();
        return dt_ctr_->deep_copy_to(ShortTypeCode::of<Parameter>(), dedup);
    }

    int32_t compare(const Parameter& other) const
    {
        if (is_not_null() && other.is_not_null())
        {
            return view().compare(other.view());
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Comparing operands may not be nullptr").do_throw();
        }
    }


    bool equals(const Parameter& other) const
    {
        if (is_not_null() && other.is_not_null()) {
            return view() == other.view();
        }
        else {
            return is_null() == other.is_null();
        }
    }

    bool operator!=(const Parameter& other) const {
        return !equals(other);
    }

    bool operator==(const Parameter& other) const {
        return equals(other);
    }

    operator Object() const & noexcept {
        return as_object();
    }

    operator Object() && noexcept {
        return Object(this->release_mem_holder(), dt_ctr_ , MoveOwnershipTag{});
    }

    static void check_structure(const void* addr, CheckStructureState& state)
    {
        state.check_alignment<ArenaDTContainer>(addr, MA_SRC);

        const ArenaDTContainer* param
                = reinterpret_cast<const ArenaDTContainer*>(addr);
        param->check(state, MA_SRC);
    }

private:
    void assert_not_null() const
    {
        if (MMA_UNLIKELY(dt_ctr_ == nullptr)) {
            MEMORIA_MAKE_GENERIC_ERROR("ParameterView is null").do_throw();
        }
    }
};


static inline std::ostream& operator<<(std::ostream& out, const Parameter& ptr) {
    out << ptr.to_string();
    return out;
}


}}
