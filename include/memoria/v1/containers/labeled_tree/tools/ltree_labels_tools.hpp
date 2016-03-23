
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/tools/isequencedata.hpp>
#include <memoria/v1/core/packed/array/packed_fse_bitmap.hpp>
#include <memoria/v1/core/types/types.hpp>

#include <tuple>

namespace memoria   {
namespace louds     {

template<typename LabedDescr> struct LabelTypeTF;

template<typename T, Indexed idx>
struct LabelTypeTF<FLabel<T, idx>> {
    typedef T Type;
};

template<typename T, Indexed idx, Granularity gr>
struct LabelTypeTF<VLabel<T, gr, idx>> {
    typedef T Type;
};

template<Int BitsPerLabel>
struct LabelTypeTF<FBLabel<BitsPerLabel>> {
    typedef UBigInt Type;
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

}
}
