
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/bt_names.hpp>

namespace memoria {
namespace v1 {
namespace edge_map {

class CtrApiName        {};
class CtrCApiName       {};
class CtrInsertName     {};
class CtrCInsertName    {};
class CtrCNavName       {};
class CtrRemoveName     {};
class CtrCRemoveName    {};


class ItrApiName {};
class ItrCApiName {};
class ItrNavName {};
class ItrCNavName {};
class ItrMiscName {};

class ItrValueName {};
class ItrCValueName {};
class ItrMrkValueName {};

}

template <typename Types>
struct EdgeMapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct EdgeMapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using EdgeMapCtrTypes  = BTCtrTypes<EdgeMapCtrTypesT<Types>>;

template <typename Types>
using EdgeMapIterTypes = BTIterTypes<EdgeMapIterTypesT<Types>>;


}}
