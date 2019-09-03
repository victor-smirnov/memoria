
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/api/datatypes/traits.hpp>

namespace memoria {
namespace v1 {

struct PkdFSEArrayTag {};
struct PkdFSETreeTag  {};
struct PkdRLESeqTag   {};
struct PkdFSESeqTag   {};
struct PkdVLEArrayTag {};
struct PkdVLETreeTag  {};

template <typename DataType, typename PkdStructTag, typename PkdStruct>
struct PkdDataTypeAccessor;

template <typename PkdStructTag>
struct PkdStructApiTraits;

}}
