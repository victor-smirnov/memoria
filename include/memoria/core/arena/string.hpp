
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

#include <memoria/core/datatypes/varchars/varchars.hpp>

#include <memoria/core/tools/u64i_56_vlen.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <memoria/core/reflection/typehash.hpp>

#include <memoria/core/linked/common/linked_hash.hpp>

#include <memoria/core/hermes/common.hpp>

#include <new>

namespace memoria {
namespace arena {

template <typename Selector>
class ArenaDataTypeContainer<Varchar, Selector> {
    using ViewT = DTTViewType<Varchar>;

    uint8_t array[1];
public:
    static constexpr bool UseObjectSize = true;

    ArenaDataTypeContainer(ViewT view)
    {
        uint8_t* buffer = raw_data();
        size_t len = u64_56_vlen_value_size(view.length());
        encode_u64_56_vlen(buffer, view.length());
        CopyByteBuffer(view.data(), buffer + len, view.length());
    }

    static uint64_t object_size(ViewT view)
    {
        size_t len = view.length();
        size_t len_len = u64_56_vlen_value_size(len);
        return len_len + len;
    }

    U8StringView view() const noexcept
    {
        const uint8_t* buffer = raw_data();
        uint64_t size{};
        size_t len_len = decode_u64_56_vlen(buffer, size);
        return U8StringView(ptr_cast<char>(buffer + len_len), size);
    }

    bool equals_to(U8StringView str) const noexcept {
        return view() == str;
    }

    bool equals_to(const ArenaDataTypeContainer* str) const noexcept {
        return view() == str->view();
    }

    void hash_to(FNVHasher<8>& hasher) const noexcept {
        for (auto ch: view()) {
            hasher.append(ch);
        }
    }

    void stringify(std::ostream& out,
                   hermes::DumpFormatState& state,
                   hermes::DumpState& dump_state)
    {
        U8StringView kk_escaped = hermes::StringEscaper::current().escape_quotes(view());
        out << "'" << kk_escaped << "'";
        hermes::StringEscaper::current().reset();
    }

private:
    uint8_t* raw_data() noexcept {
        return std::launder<uint8_t>(array);
    }

    const uint8_t* raw_data() const noexcept {
        return std::launder<const uint8_t>(array);
    }

};

using ArenaString = ArenaDataTypeContainer<Varchar, EmptyType>;

}}
