
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

#ifdef _WIN32
#define MMA1_WINDOWS
#elif __linux__
#define MMA1_LINUX
#define MMA1_POSIX
#elif __APPLE__
#define MMA1_MACOSX
#define MMA1_POSIX
#else
#error "This platform is not explicitly supported"
#endif

#ifdef __clang__
#define MMA1_CLANG
#endif


#if defined(MMA1_WINDOWS)
#pragma warning (disable : 4503)
#pragma warning (disable : 4355)
#pragma warning (disable : 4244)
#pragma warning (disable : 4231)
#pragma warning (disable : 4800)

#pragma warning( disable : 4267)
#pragma warning( disable : 4146)
#pragma warning( disable : 4307)
#pragma warning( disable : 4200)
#pragma warning( disable : 4291)
#pragma warning( disable : 4101)
#pragma warning( disable : 4305)
#pragma warning( disable : 4018)
#pragma warning( disable : 4805)

#define MMA1_V1_ALWAYS_INLINE

#define MMA1_LIKELY(expr) (expr)
#define MMA1_UNLIKELY(expr) (expr)

#else

#define MMA1_V1_ALWAYS_INLINE __attribute__((always_inline))

#define MMA1_LIKELY(expr) __builtin_expect((expr),1)
#define MMA1_UNLIKELY(expr) __builtin_expect((expr),0)

#endif

#if defined(MMA1_CLANG)
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wunused-command-line-argument"
#endif


#if defined(MMA1_POSIX) || defined(MMA1_CLANG)
#define MMA1_NODISCARD [[nodiscard]]
#else
#define MMA1_NODISCARD
#endif


#ifndef MMA1_SOURCE

#define MMA1_JOIN( X, Y ) MMA1_INT0_JOIN( X, Y )
#define MMA1_INT0_JOIN( X, Y ) MMA1_INT1_JOIN(X,Y)
#define MMA1_INT1_JOIN( X, Y ) X##Y

#define MMA1_STRINGIFY(x) #x
#define MMA1_TOSTRING(x) MMA1_STRINGIFY(x)

#define MMA1_AT MMA1_TOSTRING(__LINE__)

#define MMA1_SOURCE (__FILE__ ":" MMA1_AT)
#define MMA1_RAW_SOURCE (__FILE__ ":" MMA1_AT)
#define MA_SRC MMA1_SOURCE
#define MMA1_SRC MMA1_SOURCE
#define MA_RAW_SRC MMA1_RAW_SOURCE

#endif


#ifndef MMA1_ICU_CXX_NS
#define MMA1_ICU_CXX_NS icu_60
#endif

