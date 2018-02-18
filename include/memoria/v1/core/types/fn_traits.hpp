
// Copyright 2014 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <tuple>

namespace memoria {
namespace v1 {

template <typename T> struct FnTraits;

template <typename RtnType_, typename ClassType_, typename... Args_>
struct FnTraits<RtnType_ (ClassType_::*)(Args_...)> {
    typedef RtnType_                RtnType;
    typedef ClassType_              ClassType;
    typedef TypeList<Args_...>      ArgsList;

    static const int32_t Arity          = sizeof...(Args_);
    static const bool Const         = false;


    template <int32_t I>
    using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;

    using Exists = void;
};

template <typename RtnType_, typename ClassType_, typename... Args_>
struct FnTraits<RtnType_ (ClassType_::*)(Args_...) const> {
    typedef RtnType_                RtnType;
    typedef ClassType_              ClassType;
    typedef TypeList<Args_...>      ArgsList;

    static const int32_t Arity          = sizeof...(Args_);
    static const bool Const         = true;

    template <int32_t I>
    using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;

    using Exists = void;
};

template <typename RtnType_, typename... Args_>
struct FnTraits<RtnType_ (*)(Args_...)> {
    typedef RtnType_                RtnType;
    typedef void                    ClassType;
    typedef TypeList<Args_...>      ArgsList;

    static const int32_t Arity          = sizeof...(Args_);
    static const bool Const         = true;

    template <int32_t I>
    using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;

    using Exists = void;
};

template <typename RtnType_, typename... Args_>
struct FnTraits<RtnType_ (Args_...)> {
    typedef RtnType_                RtnType;
    typedef void                    ClassType;
    typedef TypeList<Args_...>      ArgsList;

    static const int32_t Arity          = sizeof...(Args_);
    static const bool Const         = true;

    template <int32_t I>
    using Arg = typename std::tuple_element<I, std::tuple<Args_...>>::type;

    using Exists = void;
};


}}