
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/containers/vector/vctr_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>



namespace memoria {
namespace v1 {

using namespace v1::bt;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::mvector::CtrToolsName)
public:
    using typename Base::Types;
protected:



MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::mvector::CtrToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS


}}