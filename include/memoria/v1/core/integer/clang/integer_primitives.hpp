
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

namespace memoria {
namespace v1 {

static inline bool clang_sadd_overflow(int a, int b, int* res) {
    return __builtin_sadd_overflow(a, b, res);
}

static inline bool clang_sadd_overflow(long int a, long int b, long int* res) {
    return __builtin_saddl_overflow(a, b, res);
}

static inline bool clang_sadd_overflow(long long int a, long long int b, long long int* res) {
    return __builtin_saddll_overflow(a, b, res);
}

static inline bool clang_uadd_overflow(unsigned int a, unsigned int b, unsigned int* res) {
    return __builtin_uadd_overflow(a, b, res);
}

static inline bool clang_uadd_overflow(unsigned long int a, unsigned long int b, unsigned long int* res) {
    return __builtin_uaddl_overflow(a, b, res);
}

static inline bool clang_uadd_overflow(unsigned long long int a, unsigned long long int b, unsigned long long int* res) {
    return __builtin_uaddll_overflow(a, b, res);
}



static inline bool clang_ssub_overflow(int a, int b, int* res) {
    return __builtin_ssub_overflow(a, b, res);
}

static inline bool clang_ssub_overflow(long int a, long int b, long int* res) {
    return __builtin_ssubl_overflow(a, b, res);
}

static inline bool clang_ssub_overflow(long long int a, long long int b, long long int* res) {
    return __builtin_ssubll_overflow(a, b, res);
}

static inline bool clang_usub_overflow(unsigned int a, unsigned int b, unsigned int* res) {
    return __builtin_usub_overflow(a, b, res);
}

static inline bool clang_usub_overflow(unsigned long int a, unsigned long int b, unsigned long int* res) {
    return __builtin_usubl_overflow(a, b, res);
}

static inline bool clang_usub_overflow(unsigned long long int a, unsigned long long int b, unsigned long long int* res) {
    return __builtin_usubll_overflow(a, b, res);
}



static inline bool clang_smul_overflow(int a, int b, int* res) {
    return __builtin_smul_overflow(a, b, res);
}

static inline bool clang_smul_overflow(long int a, long int b, long int* res) {
    return __builtin_smull_overflow(a, b, res);
}

static inline bool clang_smul_overflow(long long int a, long long int b, long long int* res) {
    return __builtin_smulll_overflow(a, b, res);
}

static inline bool clang_umul_overflow(unsigned int a, unsigned int b, unsigned int* res) {
    return __builtin_umul_overflow(a, b, res);
}

static inline bool clang_umul_overflow(unsigned long int a, unsigned long int b, unsigned long int* res) {
    return __builtin_umull_overflow(a, b, res);
}

static inline bool clang_umul_overflow(unsigned long long int a, unsigned long long int b, unsigned long long int* res) {
    return __builtin_umulll_overflow(a, b, res);
}


//constexpr bool clang_check_add_overflow(int a, int b) {
//    return __builtin_add_overflow_p(a, b, (decltype(a + b))0);
//}

//constexpr bool clang_check_add_overflow(int a, int b) {
//    return __builtin_mul_overflow_p(a, b, (decltype(a + b))0);
//}



}}
