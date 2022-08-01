
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/linked/document/ld_document.hpp>

#include <fmt/format.h>

namespace memoria {

class LDStringView {
    const LDDocumentView* doc_;
    ld_::LDPtr<U8LinkedString> string_;

    friend class LDTypeDeclarationView;
    friend class LDDArrayView;
    friend class LDDMapView;

public:
    LDStringView(): doc_(), string_({}) {}

    LDStringView(const LDDocumentView* doc, ld_::LDPtr<U8LinkedString> string, LDDValueTag tag = 0):
        doc_(doc), string_(string)
    {}

    LDPtrHolder ptr() const {
        return string_.get();
    }

    ViewPtr<U8StringView> view() const noexcept {
        return doc_->wrap(string_.get(&doc_->arena_)->view());
    }

    operator LDDValueView() const noexcept;

    PoolSharedPtr<LDDocument> clone(bool compactify = true) const;

    ld_::LDPtr<U8LinkedString> deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const;

    std::ostream& dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const;

    std::ostream& dump(std::ostream& out, LDDumpFormatState& format) const
    {
        LDDumpState dump_state(*doc_);
        dump(out, format, dump_state);
        return out;
    }

    std::ostream& dump(std::ostream& out) const {
        LDDumpFormatState state;
        LDDumpState dump_state(*doc_);
        return dump(out, state, dump_state);
    }
};

std::ostream& operator<<(std::ostream& out, const LDStringView& value);

}


namespace fmt {

template <>
struct formatter<memoria::LDStringView> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::LDStringView& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.view()->to_string());
    }
};

}
