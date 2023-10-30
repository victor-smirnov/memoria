
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

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/reflection/reflection.hpp>

#include <memoria/core/linked/common/linked_hash.hpp>

#include <memoria/core/hermes/common.hpp>
#include <memoria/core/hermes/traits.hpp>



#include <new>

namespace memoria {
namespace arena {

template <typename Selector>
class ArenaDataTypeContainer<Varchar, Selector> {
    using ViewT = DTTViewType<Varchar>;

    using OViewT = Own<U8StringOView, OwningKind::HOLDING>;

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

    static uint64_t object_size(ViewT view) {
        return object_size(view.length());
    }

    static uint64_t object_size(size_t str_len)
    {
        size_t len = str_len;
        size_t len_len = u64_56_vlen_value_size(len);
        return len_len + len;
    }

    OViewT view(LWMemHolder* ptr_holder) const noexcept
    {
        const uint8_t* buffer = raw_data();
        uint64_t size{};
        size_t len_len = decode_u64_56_vlen(buffer, size);
        return OViewT(ptr_holder, ptr_cast<char>(buffer + len_len), size);
    }

    size_t object_size() const noexcept {
        const uint8_t* buffer = raw_data();
        uint64_t size{};
        size_t len_len = decode_u64_56_vlen(buffer, size);
        return len_len + size;
    }

    ViewT view() const noexcept
    {
        const uint8_t* buffer = raw_data();
        uint64_t size{};
        size_t len_len = decode_u64_56_vlen(buffer, size);
        return ViewT(ptr_cast<char>(buffer + len_len), size);
    }

    bool equals_to(U8StringView str, LWMemHolder* mem_holder) const noexcept {
        return view(mem_holder) == str;
    }

    bool equals_to(const ArenaDataTypeContainer* str, LWMemHolder* mem_holder) const noexcept {
        return view() == str->view();
    }

    void hash_to(FNVHasher<8>& hasher) const noexcept {
        for (auto ch: view()) {
            hasher.append(ch);
        }
    }

    void stringify(std::ostream& out,
                   hermes::DumpFormatState& state, LWMemHolder* mem_holder)
    {
        stringify_view(out, state, view(mem_holder));
    }

    static void stringify_view(
            std::ostream& out,
            hermes::DumpFormatState& state,
            const U8StringView& view
    ){
        if (state.cfg().use_raw_strings())
        {
            U8StringView kk_escaped = hermes::RawStringEscaper::current().escape_quotes(view);
            out << "'" << kk_escaped << "'";
            hermes::RawStringEscaper::current().reset();
        }
        else {
            U8StringView kk_escaped = hermes::StringEscaper::current().escape_chars(view);
            out << "\"" << kk_escaped << "\"";
            hermes::StringEscaper::current().reset();
        }
    }

    ArenaDataTypeContainer* deep_copy_to(
            ShortTypeCode tag,
            hermes::DeepCopyState& dedup
    ) const
    {
        auto& dst = dedup.arena();
        ArenaDataTypeContainer* str = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)str)) {
            return str;
        }
        else {
            ArenaDataTypeContainer* new_str = dst.template allocate_tagged_object<ArenaDataTypeContainer>(tag, view(dedup.mem_holder()));
            dedup.map(dst, this, new_str);
            return new_str;
        }
    }


    static void* from_view(const ViewT& view) {
        const auto* addr = view.data();
        size_t len_len = u64_56_len_len(view.length());
        return ptr_cast<void*>(addr - len_len);
    }

    void check(hermes::CheckStructureState& state, const char* src) const {
        state.check_and_set_tagged(
                this,
                object_size(),
                src
        );

        state.mark_as_processed(this);
    }

    static void check(
        const arena::EmbeddingRelativePtr<void>& ptr,
        hermes::CheckStructureState& state,
        const char* src
    ) {}

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
