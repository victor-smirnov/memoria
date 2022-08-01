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

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <memoria/core/linked/document/linked_document.hpp>
#include <memoria/core/datatypes/type_registry.hpp>

#include <memoria/core/tools/result.hpp>
#include <memoria/core/regexp/icu_regexp.hpp>


namespace memoria {

std::ostream& LDDValueView::dump(std::ostream& out, LDDumpFormatState& state, LDDumpState& dump_state) const
{
    if (is_null()) {
        out << "null";
    }
    else {
        auto ops = DataTypeRegistry::local().get_operations(type_tag_).get();
        ops->dump(doc_, value_ptr_, out, state, dump_state);
    }

    return out;
}


bool LDDValueView::is_simple_layout() const noexcept
{
    if (is_varchar() || is_bigint() || is_null() || is_boolean()) {
        return true;
    }

    if (is_map()) {
        return as_map()->is_simple_layout();
    }

    if (is_array()) {
        return as_array()->is_simple_layout();
    }

    if (is_type_decl()) {
        return as_type_decl()->is_simple_layout();
    }

    if (is_typed_value()) {
        return as_typed_value()->is_simple_layout();
    }

    return false;
}


ld_::LDDPtrHolder LDDValueView::deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping& mapping) const
{
    if (is_null()) {
        return LDDValueView(tgt, 0).value_ptr_;
    }
    else {
        auto ops = DataTypeRegistry::local().get_operations(type_tag_).get();
        return ops->deep_copy_to(doc_, value_ptr_, tgt, mapping);
    }
}


ld_::LDPtr<U8LinkedString> LDStringView::deep_copy_to(LDDocumentView* tgt, ld_::LDArenaAddressMapping&) const
{
    const LinkedString<char>* vv0 = string_.get(&doc_->arena_);
    U8StringView view = vv0->view();
    return tgt->new_varchar(view).string_;
}


std::ostream& operator<<(std::ostream& out, const LDStringView& value) {
    LDDumpFormatState format = LDDumpFormatState().simple();
    ((LDDValueView)value).dump(out, format);
    return out;
}


PoolSharedPtr<LDDocument> LDDValueView::clone(bool compactify) const
{
    PoolSharedPtr<LDDocument> tgt = LDDocument::make_new();
    ld_::LDArenaAddressMapping mapping(*doc_);

    LDDValueView tgt_value(tgt.get(), deep_copy_to(tgt.get(), mapping));
    tgt->set_doc_value(tgt_value);

    if (compactify) {
        return tgt->compactify();
    }

    return tgt;
}

PoolSharedPtr<LDDocument> LDStringView::clone(bool compactify) const {
    LDDValueView vv = *this;
    return vv.clone(compactify);
}

std::ostream& LDStringView::dump(std::ostream& out, LDDumpFormatState&, LDDumpState&) const
{
    auto str = view();
    U8StringView str_escaped = SDNStringEscaper::current().escape_quotes(*str);

    out << "'" << str_escaped << "'";

    SDNStringEscaper::current().reset();

    return out;
}

std::vector<U8String> parse_path_expression(U8StringView path) {
    auto pattern = ICURegexPattern::compile(u"(/)+");
    return pattern.split(path);
}

bool find_value(LDDValueView& res, U8StringView path_str)
{
    auto path = parse_path_expression(path_str);

    for (size_t c = 0; c < path.size(); c++)
    {
        const U8String& step = path[c];
        if (!step.is_empty())
        {
            if (step == "$") {
                if (res.is_typed_value()) {
                    auto res2 = res.as_typed_value()->constructor();
                    res = std::move(*res2);
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Value has invalid type for step {} in path expression '{}'", c, step).do_throw();
                }
            }
            else if (res.is_map())
            {
                auto res2 = res.as_map()->get(step);
                if (res2.is_empty()) {
                    return false;
                }
                else {
                    res = std::move(*res2);
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Value has invalid type for step {} in path expression '{}'", c, step).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Empty step {} in the path expression '{}'", c, step).do_throw();
        }
    }

    return true;
}

ViewPtr<LDDValueView> get_value(ViewPtr<LDDValueView> src, U8StringView path) {
    if (find_value(*src, path)) {
        return src;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Value for path '{}' is not found", path).do_throw();
    }
}

}
