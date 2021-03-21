
// Copyright 2021 Victor Smirnov
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

#include "python_commons.hpp"

#include <memoria/core/tools/result.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace pybind11 { namespace detail {

template <typename Type, typename Value>
struct ResultTCaster;

template <typename Value>
struct ResultTCaster<memoria::Result<Value>, Value> {
    using type = memoria::Result<Value>;
    using value_conv = make_caster<Value>;

    bool load(handle, bool) {
        return false; // Return values only
    }

    template <typename T>
    static handle cast(T &&src, return_value_policy policy, handle parent) {
        src.throw_if_error();

        if (!std::is_lvalue_reference<T>::value) {
            policy = return_value_policy_override<Value>::policy(policy);
        }

        return value_conv::cast(forward_like<T>(src.get()), policy, parent);
    }

    PYBIND11_TYPE_CASTER(type, _("Result[") + value_conv::name + _("]"));
};


template <>
struct ResultTCaster<memoria::Result<void>, void> {
    using type = memoria::Result<void>;

    bool load(handle src, bool)
    {
        if (!pybind11::isinstance<pybind11::none>(src))
            return false;
        value = memoria::VoidResult::of();
        return true;
    }

    template <typename T>
    static handle cast(T &&src, return_value_policy, handle) {
        src.throw_if_error();
        return none().inc_ref();
    }

    PYBIND11_TYPE_CASTER(type, _("Result[Void]"));
};



template <typename T>
struct type_caster<memoria::Result<T>> : ResultTCaster<memoria::Result<T>, T> {};

}}
