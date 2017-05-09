
// Copyright 2011 Victor Smirnov
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


#ifdef _INTEL_COMPILER

#pragma warning (disable : 2586)
#pragma warning (disable : 1125)
#pragma warning (disable : 869)

#elif defined(_MSC_VER)

#pragma warning (disable : 4503)
#pragma warning (disable : 4355)
#pragma warning (disable : 4244)
#pragma warning (disable : 4231)
#pragma warning (disable : 4800)

#elif defined(__clang__)

#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wunused-private-field"

#endif


#ifndef MEMORIA_SOURCE

#define MEMORIA_JOIN( X, Y ) MEMORIA_INT0_JOIN( X, Y )
#define MEMORIA_INT0_JOIN( X, Y ) MEMORIA_INT1_JOIN(X,Y)
#define MEMORIA_INT1_JOIN( X, Y ) X##Y

#define MEMORIA_STRINGIFY(x) #x
#define MEMORIA_TOSTRING(x) MEMORIA_STRINGIFY(x)

#define MEMORIA_AT MEMORIA_TOSTRING(__LINE__)

#define MEMORIA_SOURCE (__FILE__ ":" MEMORIA_AT)
#define MEMORIA_RAW_SOURCE (__FILE__ ":" MEMORIA_AT)
#define MA_SRC MEMORIA_SOURCE
#define MMA1_SRC MEMORIA_SOURCE
#define MA_RAW_SRC MEMORIA_RAW_SOURCE

#endif
