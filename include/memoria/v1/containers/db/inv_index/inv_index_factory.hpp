
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/prototypes/bt_fl/btfl_factory.hpp>

namespace memoria {
namespace v1 {

template <
    typename Profile
>
struct BTTypes<Profile, InvertedIndex>: public BTTypes<Profile, v1::BTFreeLayout>{};


template <typename Profile, typename T>
class CtrTF<Profile, InvertedIndex, T>: public CtrTF<Profile, v1::BTFreeLayout, T> {
};



}}
