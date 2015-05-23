
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_FACTORY_HPP
#define _MEMORIA_CONTAINERS_SEQD_FACTORY_HPP

#include <memoria/containers/seq_dense/factory/seqd_1_factory.hpp>
#include <memoria/containers/seq_dense/factory/seqd_8_factory.hpp>

namespace memoria {

template <typename Profile, Int BitsPerSymbol, bool Dense, typename T>
class CtrTF<Profile, memoria::Sequence<BitsPerSymbol, Dense>, T>: public CtrTF<Profile, memoria::BTSingleStream, T> {
};

}

#endif
