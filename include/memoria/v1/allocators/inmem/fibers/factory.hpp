
// Copyright 2016 Victor Smirnov
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
#include <dumbo/v1/tools/types.hpp>
#include <dumbo/v1/tools/shared_ptr.hpp>
#include "allocator.hpp"


#include <core/shared_ptr.hh>
#include "../../tools/shared_ptr_ns.hpp"

namespace dumbo {
namespace v1 {

template <typename T>
struct SeastarMakeSharedPtr {
	template <typename... Args>
	static auto make_shared(Args&&... args) {
		return ns::make_shared<T>(std::forward<Args>(args)...);
	}
};

template <typename Profile>
class DumboContainerCollectionCfg: public memoria::v1::BasicContainerCollectionCfg<Profile> {
public:
    template <typename T>
    using CtrSharedPtr = ns::shared_ptr<T>;

    template <typename T>
    using CtrEnableSharedFromThis = ns::enable_shared_from_this<T>;

    template <typename T>
    using CtrMakeSharedPtr = SeastarMakeSharedPtr<T>;
};

}}


namespace memoria {
namespace v1 {

template <typename Profile>
class ContainerCollectionCfg;

template <typename T>
class ContainerCollectionCfg<dumbo::v1::DumboProfile<T> > {
public:
    using Types = dumbo::v1::DumboContainerCollectionCfg<dumbo::v1::DumboProfile<T>>;
};






template <typename CtrName>
using DumboCtrTF = CtrTF<dumbo::v1::DumboProfile<>, CtrName>;

template <typename CtrName>
using DumboCtr = typename CtrTF<dumbo::v1::DumboProfile<>, CtrName>::Type;

template <typename CtrName>
void DumboInit() {
    DumboCtr<CtrName>::initMetadata();
}

}}
