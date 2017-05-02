
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/containers/seq_dense/factory/seqd_1_factory.hpp>
#include <memoria/v1/containers/seq_dense/factory/seqd_8_factory.hpp>

namespace memoria {
namespace v1 {

template <typename Profile, int32_t BitsPerSymbol, bool Dense, typename T>
class CtrTF<Profile, v1::Sequence<BitsPerSymbol, Dense>, T>: public CtrTF<Profile, v1::BTSingleStream, T> {
};

}}