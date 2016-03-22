
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/types.hpp>


namespace memoria    {

template <typename T, T V1, T V2>
struct Max {
    static const T Value = V1 > V2 ? V1 : V2;
};

template <typename T, T V1, T V2>
struct Min {
    static const T Value = V1 < V2 ? V1 : V2;
};

}

