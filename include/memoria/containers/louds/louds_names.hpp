
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CONTAINERS_LOUDS_NAMES_HPP_
#define MEMORIA_CONTAINERS_LOUDS_NAMES_HPP_

namespace memoria    {
namespace louds	 	 {

class CtrApiName {};
class ItrApiName {};

}

template <typename Types>
struct LoudsCtrTypes: CtrTypesT<Types> {};

template <typename Types>
struct LoudsIterTypes: IterTypesT<Types> {};



}

#endif
