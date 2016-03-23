
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/core/types/list/typelist.hpp>

namespace memoria {
namespace v1 {

template <typename Type>
struct IsList: ConstValue<bool, false> {};

template <typename ... List>
struct IsList<TypeList<List...> >: ConstValue<bool, true> {};


template <typename Type>
struct IsNonemptyList: ConstValue<bool, false> {};

template <typename ... List>
struct IsNonemptyList<TypeList<List...>>: ConstValue<bool, true> {};

template <>
struct IsNonemptyList<TypeList<>>: ConstValue<bool, false> {};

}}