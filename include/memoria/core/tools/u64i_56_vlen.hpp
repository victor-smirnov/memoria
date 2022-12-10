
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/tools/result.hpp>

namespace memoria {

static inline constexpr size_t u64_56_vlen_value_size(uint64_t value) {
  // Optimizing for small values;
  if (MMA_LIKELY(value < 249)) {
    return 1;
  }

  uint64_t mask = 0x00FF000000000000;
  for (size_t c = 8; c > 0; c--) {
    if (MMA_UNLIKELY((bool)(mask & value))) {
      return c;
    }

    mask >>= 8;
  }

  return 1 + (value >= 249);
}

static inline constexpr size_t encode_u64_56_vlen_noex(uint8_t* array, uint64_t value) noexcept
{
  // Optimizing for small values;
  if (MMA_LIKELY(value < 249)) {
    *array = (uint8_t)value;
    return 1;
  }

  // 56 bit max
  if (value <= 0xFFFFFFFFFFFFFF)
  {
    uint64_t mask = 0x00FF000000000000;
    size_t c{};
    for (c = 7; c > 0; c--) {
      if (MMA_UNLIKELY((bool)(mask & value))) {
        break;
      }

      mask >>= 8;
    }

    *array = c + 248;

    for (size_t d = 1; d <= c; d++) {
      *(array + d) = value >> ((d - 1) * 8);
    }

    return c + 1;
  }
  else {
    return 0;
  }
}

static inline size_t encode_u64_56_vlen(uint8_t* array, uint64_t value)
{
  size_t size = encode_u64_56_vlen_noex(array, value);
  if (MMA_LIKELY(size > 0)) {
    return value;
  }
  else {
    MEMORIA_MAKE_GENERIC_ERROR("Provided ui64_56 value of {} is out of range", value).do_throw();
  }
}

static inline size_t u64_56_len_len(size_t len)
{
  if (MMA_LIKELY(len < 249)) {
    return 1;
  }
  else {
    uint64_t mask = 0x00FF000000000000;
    size_t c{};
    for (c = 7; c > 0; c--) {
        if (MMA_UNLIKELY((bool)(mask & len))) {
            break;
        }

        mask >>= 8;
    }

    return c + 1;
  }
}


static inline constexpr size_t decode_u64_56_vlen(const uint8_t* array, uint64_t& value) noexcept
{
  // Optimizing for small values;
  if (MMA_LIKELY(*array < 249)) {
    value = *array;
    return 1;
  }

  size_t size = *array - 248;
  for (size_t c = 0; c < size; c++) {
    uint64_t token = *(array + c + 1);
    value |= token << (c * 8);
  }

  return size;
}


static inline constexpr uint64_t decode_u64_56_vlen(const uint8_t* array) noexcept
{
  uint64_t value{};
  decode_u64_56_vlen(array, value);
  return value;
}

}
