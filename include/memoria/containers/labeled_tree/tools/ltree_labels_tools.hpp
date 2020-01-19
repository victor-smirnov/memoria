
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

#include <memoria/core/tools/isequencedata.hpp>
#include <memoria/core/packed/array/packed_fse_bitmap.hpp>
#include <memoria/core/types.hpp>

#include <tuple>

namespace memoria {
namespace louds {

template<typename LabedDescr> struct LabelTypeTF;

template<typename T, Indexed idx>
struct LabelTypeTF<FLabel<T, idx>> {
    typedef T Type;
};

template<typename T, Indexed idx, Granularity gr>
struct LabelTypeTF<VLabel<T, gr, idx>> {
    typedef T Type;
};

template<int32_t BitsPerLabel>
struct LabelTypeTF<FBLabel<BitsPerLabel>> {
    typedef uint64_t Type;
};








template <typename... Labels> class LabelsTypeListBuilder;

template <typename Head, typename... Tail>
class LabelsTypeListBuilder<Head, Tail...> {
    typedef typename LabelTypeTF<Head>::Type                                    LabelDataType;
public:
    typedef typename PrependToList<
            typename LabelsTypeListBuilder<Tail...>::Type,
            LabelDataType
    >::Type                                                                     Type;
};

template <>
class LabelsTypeListBuilder<> {
public:
    typedef TypeList<> Type;
};

template <typename... Labels>
class LabelsTypeListBuilder<TypeList<Labels...> > {
public:
    typedef typename LabelsTypeListBuilder<Labels...>::Type                     Type;
};




template <typename... Labels>
struct TupleTF {
    typedef std::tuple<Labels...>                                               Type;
};


template <typename... Labels>
struct TupleTF<TypeList<Labels...>> {
    typedef std::tuple<Labels...>                                               Type;
};


template <typename... LabelDescriptors>
class LabelsTupleTF {
    typedef typename LabelsTypeListBuilder<LabelDescriptors...>::Type           LabelTypesList;

public:
    typedef typename TupleTF<LabelTypesList>::Type                              Type;
};

}}
