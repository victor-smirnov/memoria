/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
** Copyright (c) 2022 Victor Smirnov
**
** This file is originally based on the jmespath.cpp project
** (https://github.com/robertmrk/jmespath.cpp, commitid: 9c9702a)
** and is distributed under the MIT License (MIT).
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
****************************************************************************/


#pragma once

#define BOOST_SPIRIT_UNICODE

#include <memoria/core/hermes/path/types.h>
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix.hpp>

namespace memoria::hermes::parser {

namespace qi = boost::spirit::qi;
namespace encoding = qi::unicode;
namespace phx = boost::phoenix;

template <
    typename Iterator,
    typename Skipper,
    typename AppendUtf8Action,
    typename AppendEscapeSequenceAction,
    typename EncodeSurrogatePairAction,
    typename String,
    typename RawString,
    typename Identifier,
    typename Literal
>
struct StringsRuleSet {

    using UnicodeChar = memoria::hermes::path::UnicodeChar;

    StringsRuleSet()
    {
        using encoding::char_;
        using qi::lit;
        using qi::lexeme;
        using qi::int_parser;
        using qi::_val;
        using qi::_1;
        using qi::_2;
        using qi::_pass;
        using qi::_r1;
        using qi::_r2;
        using qi::eps;
        using qi::_a;
        using phx::at_c;
        using phx::if_;
        using phx::ref;

        // lazy function for appending UTF-32 characters to a string encoded
        // in UTF-8
        phx::function<AppendUtf8Action> appendUtf8;
        // lazy function for appending UTF-32 encoded escape sequence to a
        // string encoded in UTF-8
        phx::function<AppendEscapeSequenceAction> appendEscape;
        // lazy function for for combining surrogate pair characters into a
        // single codepoint
        phx::function<EncodeSurrogatePairAction> encodeSurrogatePair;

        // match zero or more literal characters enclosed in grave accents
        m_literalRule = lexeme[ lit('\x60')
                >> *m_literalCharRule[appendUtf8(at_c<0>(_val), _1)]
                >> lit('\x60') ];

        // match a character in the range of 0x00-0x5B or 0x5D-0x5F or
        // 0x61-0x10FFFF or a literal escape
        m_literalCharRule = char_(U'\x00', U'\x5B')
                | char_(U'\x5D', U'\x5F')
                | char_(U'\x61', U'\U0010FFFF')
                | m_literalEscapeRule;

        // match a grave accent preceded by an escape or match a backslash if
        // it's not followed by a grave accent
        m_literalEscapeRule = (m_escapeRule >> char_(U'\x60'))
                               | (char_(U'\\') >> & (!lit('`')));

        // match zero or more raw string characters enclosed in apostrophes
        m_rawStringRule = lexeme[ lit("\'")
                >> * (m_rawStringEscapeRule[appendEscape(at_c<0>(_val), _1)]
                      | m_rawStringCharRule[appendUtf8(at_c<0>(_val), _1)])
                >> lit("\'") ];

        // match a single character in the range of 0x07-0x0D or 0x20-0x26 or
        // 0x28-0x5B or 0x5D-0x10FFFF or an escaped apostrophe
        m_rawStringCharRule = (char_(U'\x07', U'\x0D')
                               | char_(U'\x20', U'\x26')
                               | char_(U'\x28', U'\x5B')
                               | char_(U'\x5D', U'\U0010FFFF'));

        // match escape sequences
        m_rawStringEscapeRule = char_(U'\\') >>
                (char_(U'\x07', U'\x0D')
                | char_(U'\x20', U'\U0010FFFF'));

        // match unquoted or quoted strings
        m_identifierRule = m_unquotedStringRule | m_quotedStringRule;

        // match a single character in the range of 0x41-0x5A or 0x61-0x7A
        // or 0x5F (A-Za-z_) followed by zero or more characters in the range of
        // 0x30-0x39 (0-9) or 0x41-0x5A (A-Z) or 0x5F (_) or 0x61-0x7A (a-z)
        // and append them to the rule's string attribute encoded as UTF-8
        m_unquotedStringRule
            = lexeme[ ((char_(U'\x41', U'\x5A')
                        | char_(U'\x61', U'\x7A')
                        | char_(U'\x5F') | encoding::alpha )[appendUtf8(_val, _1)]
            >> *(char_(U'\x30', U'\x39')
                 | char_(U'\x41', U'\x5A')
                 | char_(U'\x5F')
                 | char_(U'\x61', U'\x7A') | encoding::alnum )[appendUtf8(_val, _1)]) ]
                - "null" - "true" - "false" - "const" - "volatile" - "signed"
                - "unsigned" - "int" - "long" - "char" - "double" - "float" - "short" - "bool" - "const";

        // match unescaped or escaped characters enclosed in quotes one or more
        // times and append them to the rule's string attribute encoded as UTF-8
        m_quotedStringRule
            = lexeme[ m_quoteRule
            >> +(m_unescapedCharRule
                 | m_escapedCharRule)[appendUtf8(_val, _1)]
            >> m_quoteRule ];

        // match characters in the range of 0x20-0x21 or 0x23-0x5B or
        // 0x5D-0x10FFFF
        m_unescapedCharRule = char_(U'\x20', U'\x21')
            | char_(U'\x23', U'\x5B')
            | char_(U'\x5D', U'\U0010FFFF');

        // match quotation mark literal
        m_quoteRule = lit('\"');

        // match backslash literal
        m_escapeRule = lit('\\');

        // match an escape character followed by quotation mark or backslash or
        // slash or control character or surrogate pair or a single unicode
        // escape
        m_escapedCharRule = lexeme[ m_escapeRule
            >> (char_(U'\"')
                | char_(U'\\')
                | char_(U'/')
                | m_controlCharacterSymbols
                | m_surrogatePairCharacterRule
                | m_unicodeCharRule) ];

        // match a pair of unicode character escapes separated by an escape
        // symbol if the first character's value is between 0xD800-0xDBFF
        // and convert them into a single codepoint
        m_surrogatePairCharacterRule
            = lexeme[ (m_unicodeCharRule >> m_escapeRule >> m_unicodeCharRule)
                [_pass = (_1 >= 0xD800 && _1 <= 0xDBFF),
                _val = encodeSurrogatePair(_1, _2)] ];

        // match a unicode character escape and convert it into a
        // single codepoint
        m_unicodeCharRule = lexeme[ lit('u')
            >> int_parser<UnicodeChar, 16, 4, 4>() ];

        // convert symbols into control characters
        m_controlCharacterSymbols.add
        (U"b", U'\x08')     // backspace
        (U"f", U'\x0C')     // form feed
        (U"n", U'\x0A')     // line feed
        (U"r", U'\x0D')     // carriage return
        (U"t", U'\x09');    // tab
    }

    qi::rule<Iterator, Identifier(), Skipper> m_identifierRule;
    qi::rule<Iterator, RawString(), Skipper>  m_rawStringRule;
    qi::rule<Iterator, Literal(), Skipper>    m_literalRule;

    qi::rule<Iterator, UnicodeChar()>       m_literalCharRule;
    qi::rule<Iterator, UnicodeChar()>       m_literalEscapeRule;
    qi::rule<Iterator, UnicodeChar()>       m_rawStringCharRule;
    qi::rule<Iterator,
             std::pair<UnicodeChar,
                       UnicodeChar>()>      m_rawStringEscapeRule;
    qi::rule<Iterator, String()>            m_quotedStringRule;
    qi::rule<Iterator, String()>            m_unquotedStringRule;
    qi::rule<Iterator, UnicodeChar()>       m_unescapedCharRule;
    qi::rule<Iterator, UnicodeChar()>       m_escapedCharRule;
    qi::rule<Iterator, UnicodeChar()>       m_unicodeCharRule;
    qi::rule<Iterator, UnicodeChar()>       m_surrogatePairCharacterRule;
    qi::rule<Iterator>                      m_quoteRule;
    qi::rule<Iterator>                      m_escapeRule;
    qi::symbols<UnicodeChar, UnicodeChar>   m_controlCharacterSymbols;
};


}
