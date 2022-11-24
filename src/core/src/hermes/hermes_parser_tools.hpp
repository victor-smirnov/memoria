
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

#include <memoria/core/hermes/hermes.hpp>
#include <boost/spirit/include/qi.hpp>


namespace memoria::hermes {

template <typename Iterator>
using ExpectationException = boost::wrapexcept<boost::spirit::qi::expectation_failure<Iterator> >;


class ErrorMessageResolver {
    using MessageProducerFn = std::function<U8String (size_t, size_t, size_t, U8StringView)>;

    ska::flat_hash_map<U8String, MessageProducerFn> map_;
public:
    ErrorMessageResolver() noexcept {

    }

    U8String get_message(U8StringView rule_name, size_t row, size_t column, size_t abs_pos, U8StringView context) noexcept
    {
        auto ii = map_.find(rule_name);
        if (ii != map_.end()) {
            return ii->second(row, column, abs_pos, context);
        }

        return fmt::format("Parse error in Hermes document, line {}:{}, pos: {}, at: {}", row, column, abs_pos, context);
    }

    static ErrorMessageResolver& instance() noexcept {
        static ErrorMessageResolver resolver;
        return resolver;
    }

    struct ErrDescripton {
        size_t line;
        size_t column;
        size_t abs;
        U8String context;
    };

    template <typename Iterator>
    ErrDescripton process_spirit_expect_exception(
            Iterator start,
            Iterator err_head,
            Iterator end
    )
    {
        //start, ex.first, ex.last
        ptrdiff_t pos = std::distance(start, err_head);

        size_t line = 1;
        size_t column = 1;

        while (start != err_head) {
            switch(*start) {
            case '\t': column += 4; break;
            case '\n': line++; column = 1; break;
            case '\r': break;
            default: column++;
            }

            start++;
        }

        U8String buf;
        bool add_ellipsis = true;
        for (size_t c = 0; c < 125; c++)
        {
            auto out_ii = std::back_inserter(buf.to_std_string());
            boost::utf8_output_iterator<decltype(out_ii)> utf8_out_ii(out_ii);
            *utf8_out_ii++ = *err_head;

            if (++err_head == end) {
                add_ellipsis = false;
                break;
            }
        }

        if (add_ellipsis) {
            buf += "...";
        }

        return ErrDescripton{line, column, (size_t)pos, buf};
    }

    template <typename Iterator>
    void do_throw(
            Iterator start,
            const ExpectationException<Iterator>& ex
    ) {
        ErrDescripton descr = process_spirit_expect_exception(start, ex.first, ex.last);
        U8String message = this->get_message(ex.what_.tag, descr.line, descr.column, descr.abs, descr.context);
        MEMORIA_MAKE_GENERIC_ERROR("{}", message).do_throw();
    }

    template <typename Iterator>
    void do_throw(
            Iterator start,
            Iterator err_head,
            Iterator end
    ) {
        ErrDescripton descr = process_spirit_expect_exception(start, err_head, end);
        U8String message = this->get_message("unknown-node", descr.line, descr.column, descr.abs, descr.context);
        MEMORIA_MAKE_GENERIC_ERROR("{}", message).do_throw();
    }
};

}
