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
#include <memoria/core/hermes/path/path.h>
#include "ast/allnodes.h"

#include <catch2/catch.hpp>

TEST_CASE("Expression")
{
    using namespace memoria::hermes::path;
    using namespace fakeit;

    SECTION("can be constructed")
    {
        SECTION("without parameters")
        {
            REQUIRE_NOTHROW(Expression{});
        }

        SECTION("from another expression")
        {
            Expression exp1{"id"};

            Expression exp2{exp1};

            REQUIRE(exp1 == exp2);
        }

        SECTION("from an rvalue expression")
        {
            Expression exp1{"id"};

            REQUIRE_NOTHROW(Expression{std::move(exp1)});
        }

        SECTION("from a valid expression string")
        {
            REQUIRE_NOTHROW(Expression{"\"id[5]\""});
        }

        SECTION("with literal")
        {
            using namespace memoria::hermes::path::literals;

            Expression expr = "id"_jmespath;

            REQUIRE(expr.toString() == "id");
        }
    }

    SECTION("can be copy assigned")
    {
        Expression exp1{"id"};
        Expression exp2;

        exp2 = exp1;

        REQUIRE(exp2 == exp1);
    }

    SECTION("can be move assigned")
    {
        Expression exp1{"id"};
        Expression exp2;

        REQUIRE_NOTHROW(exp2 = std::move(exp1));
    }

    SECTION("toString returns expression string")
    {
        String expressionString{"\"id\"[5]"};
        Expression expression{expressionString};

        REQUIRE(expression.toString() == expressionString);
    }

    SECTION("can be assigned with expression string")
    {
        String expressionString{"\"id\"[5]"};
        Expression expression;

        expression = expressionString;

        REQUIRE(expression.toString() == expressionString);
    }

    SECTION("can be move assigned with expression string")
    {
        String expressionString{"\"id\"[5]"};
        String expressionStringCopy{expressionString};
        Expression expression;

        expression = std::move(expressionStringCopy);

        REQUIRE(expression.toString() == expressionString);
    }

    SECTION("can be assigned with expression string literal")
    {
        const char* expressionString = "\"id\"[5]";
        Expression expression;

        expression = expressionString;

        REQUIRE(expression.toString() == expressionString);
    }

    SECTION("throws when constructed from an invalid expression string")
    {
        REQUIRE_THROWS_AS(Expression{"\"id\"["}, SyntaxError);
    }

    SECTION("throws when assigned with an invalid expression string")
    {
        String expressionString{"\"id\"["};
        Expression expression;

        REQUIRE_THROWS_AS(expression = expressionString, SyntaxError);
    }

    SECTION("throws when move assigned with an invalid expression string")
    {
        String expressionString{"\"id\"["};
        Expression expression;

        REQUIRE_THROWS_AS(expression = std::move(expressionString),
                          SyntaxError);
    }

    SECTION("is comparable")
    {
        Expression exp1{"\"id\""};
        Expression exp2{exp1};
        Expression exp3{"\"other\""};

        REQUIRE(exp1 == exp1);
        REQUIRE(exp1 == exp2);
        REQUIRE_FALSE(exp1 == exp3);
    }

    SECTION("isEmtpy returns true if uninitialized")
    {
        Expression exp;

        REQUIRE(exp.isEmpty());
    }

    SECTION("isEmtpy returns false if initialized")
    {
        Expression exp{"\"id\""};

        REQUIRE_FALSE(exp.isEmpty());
    }

    SECTION("isEmpty returns true if expression contains only whitespaces")
    {
        Expression exp{" \t\t\n "};

        REQUIRE(exp.isEmpty());
    }

    SECTION("return valid pointer for astRoot")
    {
        Expression exp;

        REQUIRE_FALSE(exp.astRoot() == nullptr);
    }
}
