
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>


#include <memoria/v1/core/exceptions/base.hpp>

#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

#include <stdlib.h>

namespace memoria {
namespace v1 {

using WhatInfo  = boost::error_info<struct TagMsgInfo, U8String>;
using WhatCInfo = boost::error_info<struct TagMsgCInfo, const char*>;
using WhatSInfo = boost::error_info<struct TagMsgCInfo, std::string>;

using DetailInfo  = boost::error_info<struct TagDetailInfo, U8String>;
using DetailCInfo = boost::error_info<struct TagDetailCInfo, const char*>;
using DetailSInfo = boost::error_info<struct TagDetailCInfo, std::string>;

struct Exception: virtual MemoriaThrowable {};
struct CtrTypeException: virtual Exception {};
struct NoCtrException: virtual Exception {};
struct CtrAlreadyExistsException: virtual Exception {};

struct IOException: virtual Exception {};
struct OOMException: virtual Exception {};

struct DispatchException: virtual Exception {};
struct NullPointerException: virtual Exception {};
struct BoundsException: virtual Exception {};
struct RuntimeException: virtual Exception {};

}}
