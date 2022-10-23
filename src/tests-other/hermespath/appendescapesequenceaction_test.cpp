/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2018 Róbert Márki
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
#include "parser/appendescapesequenceaction.h"

#include <catch2/catch.hpp>

TEST_CASE("AppendEscapeSequenceAction")
{
    using namespace fakeit;
    memoria::hermes::path::parser::AppendEscapeSequenceAction action;

    SECTION("append escaped character")
    {
        memoria::hermes::path::String string;

        action(string, std::make_pair(U'\\', U'a'));

        REQUIRE(string == "\\a");
    }

    SECTION("append escaped backslash")
    {
        memoria::hermes::path::String string;

        action(string, std::make_pair(U'\\', U'\\'));

        REQUIRE(string == "\\\\");
    }

    SECTION("append escaped single quote")
    {
        memoria::hermes::path::String string;

        action(string, std::make_pair(U'\\', U'\''));

        REQUIRE(string == "'");
    }
}
