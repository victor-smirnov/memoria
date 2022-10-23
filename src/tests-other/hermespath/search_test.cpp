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

#include <catch2/catch.hpp>

TEST_CASE("Search function")
{
    using namespace memoria::hermes::path;

    SECTION("returns null if search expression is empty")
    {
        auto result = search("", "[1, 2, 3]"_json);

        REQUIRE(result.is_null());
    }

    SECTION("evaluates expression on lvalue const ref")
    {
        String identifier{"identifier"};
        String value{"value"};
        Json document{{identifier, value}};
        String expression = identifier;
        Json expectedResult = value;
        REQUIRE(document.is_object());
        REQUIRE(expectedResult.is_string());

        auto result = search(expression, static_cast<const Json&>(document));

        REQUIRE(result == expectedResult);
    }

    SECTION("evaluates expression on lvalue ref")
    {
        String identifier{"identifier"};
        String value{"value"};
        Json document{{identifier, value}};
        String expression = identifier;
        Json expectedResult = value;
        REQUIRE(document.is_object());
        REQUIRE(expectedResult.is_string());

        auto result = search(expression, static_cast<Json&>(document));

        REQUIRE(result == expectedResult);
    }

    SECTION("evaluates expression on rvalue ref")
    {
        String identifier{"identifier"};
        String value{"value"};
        Json document{{identifier, value}};
        String expression = identifier;
        Json expectedResult = value;
        REQUIRE(document.is_object());
        REQUIRE(expectedResult.is_string());

        auto result = search(expression, std::move(document));

        REQUIRE(result == expectedResult);
    }
}
