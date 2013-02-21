
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CONTAINERS_LOUDS_NAMES_HPP_
#define MEMORIA_CONTAINERS_LOUDS_NAMES_HPP_

#include <memoria/prototypes/ctr_wrapper/ctrwrapper_names.hpp>

namespace memoria    {
namespace louds	 	 {

class CtrApiName {};
class CtrFindName {};

class ItrApiName {};

}

template <typename Types>
struct LoudsCtrTypesT: CtrTypesT<Types> {};

template <typename Types>
struct LoudsIterTypesT: IterTypesT<Types> {};



template <typename Types>
using LoudsCtrTypes  = CtrWrapperTypes<LoudsCtrTypesT<Types>>;

template <typename Types>
using LoudsIterTypes = IterWrapperTypes<LoudsIterTypesT<Types>>;




}

#endif
