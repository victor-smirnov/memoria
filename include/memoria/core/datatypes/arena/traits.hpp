
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
#include <memoria/core/datatypes/arena/arena.hpp>


namespace memoria {
namespace arena {

class String;
class BigInteger;
class BigDecimal;
class DateTime;
class Timestamp;

template <typename> class Vector;
template <typename> class Set;
template <typename, typename> class Map;

template <typename T>
struct ArenaTypeTag;

template<>
struct ArenaTypeTag<int8_t>: HasU64Value<1> {};

template<>
struct ArenaTypeTag<uint8_t>: HasU64Value<2> {};

template<>
struct ArenaTypeTag<int16_t>: HasU64Value<3> {};

template<>
struct ArenaTypeTag<uint16_t>: HasU64Value<4> {};

template<>
struct ArenaTypeTag<int32_t>: HasU64Value<5> {};

template<>
struct ArenaTypeTag<uint32_t>: HasU64Value<6> {};

template<>
struct ArenaTypeTag<int64_t>: HasU64Value<7> {};

template<>
struct ArenaTypeTag<uint64_t>: HasU64Value<8> {};

template<>
struct ArenaTypeTag<float>: HasU64Value<9> {};

template<>
struct ArenaTypeTag<double>: HasU64Value<10> {};

template<>
struct ArenaTypeTag<SegmentOffset>: HasU64Value<11> {};

template<>
struct ArenaTypeTag<Vector<int>>: HasU64Value<12> {};


class ArenaOffsetMapping;

struct ArenaTypeMeta {
    virtual ~ArenaTypeMeta() noexcept = default;
    virtual SegmentOffset deep_copy_to(const MemorySegment& src_buf, const SegmentOffset& ptr, ArenaSegment& dst_buf) const = 0;
};

ArenaTypeMeta& get_arena_type_metadata(arena::TypeTag tag);
bool has_arena_type_metadata(arena::TypeTag tag);

}}
