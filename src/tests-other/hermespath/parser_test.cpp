/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
**
** This file is part of the jmespath.cpp project which is distributed under
** the MIT License (MIT).
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
#include "boost/fakeit.hpp"
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix.hpp>
#include "parser/parser.h"

#include <catch2/catch.hpp>

namespace qi = boost::spirit::qi;
namespace encoding = qi::ascii;
namespace phx = boost::phoenix;
using namespace memoria::hermes::path;

template <typename Iterator, typename Skipper = encoding::space_type>
class GrammarStub : public qi::grammar<Iterator, String(), Skipper>
{
    public:
    static bool s_grammarUsed;

    GrammarStub() : GrammarStub::base_type(m_rule)
    {
        m_rule = *encoding::alpha[phx::bind(&GrammarStub::action, this)];
    }

    private:
    qi::rule<Iterator, String(), Skipper> m_rule;

    virtual void action()
    {
        s_grammarUsed = true;
    }
};
template <typename Iterator, typename Skipper>
bool GrammarStub<Iterator, Skipper>::s_grammarUsed = false;

TEST_CASE("Parser")
{
    namespace qi = boost::spirit::qi;
    namespace encoding = qi::ascii;
    using memoria::hermes::path::parser::Parser;

    Parser<GrammarStub> parser;
    Parser<GrammarStub>::GrammarType::s_grammarUsed = false;

    SECTION("uses specified grammar for parsing")
    {
        REQUIRE_FALSE(Parser<GrammarStub>::GrammarType::s_grammarUsed);

        parser.parse("a");

        REQUIRE(Parser<GrammarStub>::GrammarType::s_grammarUsed);
    }

    SECTION("throws exception on syntax error")
    {
        REQUIRE_THROWS_AS(parser.parse("abc1def"), SyntaxError);
    }

    SECTION("syntax error exception contains error location")
    {
        int location = 0;

        try
        {
            parser.parse("abc1def");
        }
        catch(SyntaxError& exception)
        {
            location = *boost::get_error_info<
                    InfoSyntaxErrorLocation>(exception);
        }

        REQUIRE(location == 3);
    }

    SECTION("syntax error exception contains search expression")
    {
        String searchExpression{"abc1def"};
        String searchExpressionInException;

        try
        {
            parser.parse(searchExpression);
        }
        catch(SyntaxError& exception)
        {
            searchExpressionInException = *boost::get_error_info<
                    InfoSearchExpression>(exception);
        }

        REQUIRE(searchExpressionInException == searchExpression);
    }
}
