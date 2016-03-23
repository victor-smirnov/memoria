
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/containers/seq_dense/factory/seqd_1_factory.hpp>
#include <memoria/v1/containers/seq_dense/factory/seqd_8_factory.hpp>

namespace memoria {
namespace v1 {

template <typename Profile, Int BitsPerSymbol, bool Dense, typename T>
class CtrTF<Profile, v1::Sequence<BitsPerSymbol, Dense>, T>: public CtrTF<Profile, v1::BTSingleStream, T> {
};

}}