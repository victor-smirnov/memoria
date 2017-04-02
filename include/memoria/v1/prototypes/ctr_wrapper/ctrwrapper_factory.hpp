
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

#include <memoria/v1/prototypes/bt/bt_names.hpp>

#include <memoria/v1/prototypes/ctr_wrapper/container/ctrwrp_c_base.hpp>
#include <memoria/v1/prototypes/ctr_wrapper/iterator/ctrwrp_i_base.hpp>

#include <memoria/v1/prototypes/ctr_wrapper/ctrwrapper_names.hpp>
#include <memoria/v1/prototypes/ctr_wrapper/iterator.hpp>

namespace memoria {
namespace v1 {

template <typename Profile_, typename ContainerTypeSelector>
struct WrapperTypes {

    typedef ContainerTypeSelector                                               ContainerTypeName;

    typedef Profile_                                                            Profile;

    typedef TypeList<
        v1::bt::AllocatorName
    >                                                                           ContainerPartsList;

    typedef TypeList<>                                                          IteratorPartsList;

    typedef EmptyType                                                           IteratorInterface;

    template <typename Types_>
    struct IterBaseFactory {
        typedef IteratorBase<Types_>                        Type;
    };

    template <typename Types_>
    struct CtrBaseFactory {
        typedef CtrWrapperCtrBase1<Types_>                  Type;
    };
};



template <typename Profile_, typename T, typename CtrName>
class CtrTF<Profile_, CtrWrapper<CtrName>, T> {

    typedef CtrTF<Profile_, CtrWrapper<CtrName>, T>                             MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef WrapperTypes<Profile_, CtrWrapper<CtrName>>                         TypesBase;

public:

    struct Types: TypesBase {
        using Allocator = typename MyType::Allocator;
    };
};

}}