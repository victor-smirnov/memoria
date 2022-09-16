
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

template <typename, typename> class Map;
template <typename> class Vector;

template <typename T>
struct ArenaTypeTag;

template <uint64_t TagValue>
using SimpleObjectTypeTag = HasU64Value<TagValue * 2>;

template<>
struct ArenaTypeTag<int8_t>: SimpleObjectTypeTag<1> {};

template<>
struct ArenaTypeTag<uint8_t>: SimpleObjectTypeTag<2> {};

template<>
struct ArenaTypeTag<int16_t>: SimpleObjectTypeTag<3> {};

template<>
struct ArenaTypeTag<uint16_t>: SimpleObjectTypeTag<4> {};

template<>
struct ArenaTypeTag<int32_t>: SimpleObjectTypeTag<5> {};

template<>
struct ArenaTypeTag<uint32_t>: SimpleObjectTypeTag<6> {};

template<>
struct ArenaTypeTag<int64_t>: SimpleObjectTypeTag<7> {};

template<>
struct ArenaTypeTag<uint64_t>: SimpleObjectTypeTag<8> {};

template<>
struct ArenaTypeTag<float>: SimpleObjectTypeTag<9> {};

template<>
struct ArenaTypeTag<double>: SimpleObjectTypeTag<10> {};

template<>
struct ArenaTypeTag<SegmentOffset>: SimpleObjectTypeTag<11> {};

template<typename T>
struct ArenaTypeTag<Vector<T>>: HasU64Value< 0x1ull + (ArenaTypeTag<T>::Value << 8)> {};

template<typename K, typename V>
struct ArenaTypeTag<Map<K, V>>: HasU64Value< 0x3ull + (ArenaTypeTag<K>::Value << 8) + (ArenaTypeTag<V>::Value << 16)> {};


class ArenaOffsetMapping;

struct ArenaTypeMeta {
    virtual ~ArenaTypeMeta() noexcept = default;
    virtual SegmentOffset deep_copy_to(const MemorySegment& src_buf, const SegmentOffset& ptr, ArenaSegment& dst_buf) const = 0;
};

ArenaTypeMeta& get_arena_type_metadata(arena::TypeTag tag);
bool has_arena_type_metadata(arena::TypeTag tag);

}}
