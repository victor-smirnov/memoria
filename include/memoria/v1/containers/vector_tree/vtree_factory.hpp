
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


#include <memoria/v1/containers/vector_tree/vtree_names.hpp>
#include <memoria/v1/containers/vector_tree/vtree_tools.hpp>
#include <memoria/v1/containers/vector_tree/vtree_iterator.hpp>
#include <memoria/v1/containers/vector_tree/vtree_walkers.hpp>

#include <memoria/v1/containers/vector_tree/container/vtree_c_base.hpp>
#include <memoria/v1/containers/vector_tree/container/vtree_c_api.hpp>


#include <memoria/v1/containers/vector_tree/iterator/vtree_i_api.hpp>

#include <memoria/v1/containers/labeled_tree/ltree_factory.hpp>
#include <memoria/v1/containers/vector/vctr_factory.hpp>

#include <memoria/v1/prototypes/composite/comp_factory.hpp>



namespace memoria {
namespace v1 {



template <typename Profile_>
struct CompositeTypes<Profile_, VTree>: public CompositeTypes<Profile_, Composite> {

    typedef VTree                                                               ContainerTypeName;

    typedef CompositeTypes<Profile_, Composite>                                 Base;

    using CtrList = MergeLists<
            typename Base::ContainerPartsList,

            vtree::CtrApiName
    >;

    using IterList = MergeLists<
            typename Base::IteratorPartsList,

            vtree::ItrApiName
    >;


    template <typename Types_>
    struct CtrBaseFactory {
        typedef vtree::VTreeCtrBase<Types_>                                     Type;
    };
};


template <typename Profile_, typename T>
class CtrTF<Profile_, VTree, T> {

    typedef CtrTF<Profile_, VTree, T>                                           MyType;

    typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator Allocator;

    typedef CompositeTypes<Profile_, VTree>                                     ContainerTypes;

public:

    struct Types: public ContainerTypes
    {
        typedef Profile_                                        Profile;
        using Allocator = typename MyType::Allocator;

        typedef VTreeCtrTypes<Types>                            CtrTypes;
        typedef VTreeIterTypes<Types>                           IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;
};


}}