
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

#include <memoria/v1/prototypes/bt_cow/btcow_names.hpp>

namespace memoria {
namespace v1 {
namespace cowmap {

class CtrApiName        {};
class CtrCApiName       {};
class CtrInsertName     {};
class CtrInsertMaxName  {};
class CtrCInsertName    {};
class CtrCNavName       {};
class CtrRemoveName     {};
class CtrCRemoveName    {};


class ItrApiName {};
class ItrCApiName {};
class ItrNavName {};
class ItrNavMaxName {};
class ItrCNavName {};

class ItrValueName {};
class ItrCValueName {};
class ItrMrkValueName {};

}

template <typename Types>
struct CowMapCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct CowMapIterTypesT: IterTypesT<Types> {};



template <typename Types>
using CowMapCtrTypes  = BTCowCtrTypes<CowMapCtrTypesT<Types>>;

template <typename Types>
using CowMapIterTypes = BTCowIterTypes<CowMapIterTypesT<Types>>;


}}
