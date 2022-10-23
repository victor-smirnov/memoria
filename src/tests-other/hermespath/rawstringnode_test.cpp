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
#include "ast/rawstringnode.h"
#include "interpreter/abstractvisitor.h"

#include <catch2/catch.hpp>

TEST_CASE("RawStringNode")
{
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using memoria::hermes::path::String;
    using namespace fakeit;

    SECTION("can be default constructed")
    {
        REQUIRE_NOTHROW(RawStringNode{});
    }

    SECTION("can be constructed with string value")
    {
        String value{"name"};
        RawStringNode node{value};

        REQUIRE(node.rawString == value);
    }

    SECTION("can be compared for equality")
    {
        String value{"name"};
        RawStringNode node1{value};
        RawStringNode node2{value};

        REQUIRE(node1 == node2);
        REQUIRE(node1 == node1);
    }

    SECTION("accepts visitor")
    {
        RawStringNode node{};
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const RawStringNode*)))
                .AlwaysReturn();

        node.accept(&visitor.get());

        Verify(OverloadedMethod(visitor, visit, void(const RawStringNode*)))
                .Once();
    }
}
