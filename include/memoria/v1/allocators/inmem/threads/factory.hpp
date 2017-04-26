
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

#include <memoria/v1/containers/map/map_factory.hpp>

#include <memoria/v1/core/container/metadata_repository.hpp>

#include "container_collection_cfg.hpp"

#include "allocator.hpp"

// namespace memoria {
// namespace v1 {
// 
// template <typename Profile>
// class ContainerCollectionCfg;
// 
// template <typename T>
// class ContainerCollectionCfg<DefaultProfile<T> > {
// public:
//     using Types = BasicContainerCollectionCfg<DefaultProfile<T>>;
// };
// 
// 
// template <typename CtrName>
// using DCtrTF = CtrTF<DefaultProfile<>, CtrName>;
// 
// template <typename CtrName>
// using DCtr = typename CtrTF<DefaultProfile<>, CtrName>::Type;
// 
// template <typename CtrName>
// void DInit() {
//     DCtr<CtrName>::initMetadata();
// }
// 
// 
// 
// 
// }}
