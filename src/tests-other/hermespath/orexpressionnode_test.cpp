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
#include "ast/allnodes.h"
#include "interpreter/abstractvisitor.h"

#include <catch2/catch.hpp>

TEST_CASE("OrExpressionNode")
{
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;

    SECTION("can be constructed")
    {
        SECTION("without parameters")
        {
            REQUIRE_NOTHROW(OrExpressionNode{});
        }

        SECTION("with left and right expression")
        {
            ExpressionNode leftExpression{IdentifierNode{"id1"}};
            ExpressionNode rightExpression{IdentifierNode{"id2"}};
            OrExpressionNode node{
                leftExpression,
                rightExpression};

            REQUIRE(node.leftExpression == leftExpression);
            REQUIRE(node.rightExpression == rightExpression);
        }
    }

    SECTION("returns false for isProjection")
    {
        OrExpressionNode node;

        REQUIRE_FALSE(node.isProjection());
    }

    SECTION("returns true for stopsProjection")
    {
        OrExpressionNode node;

        REQUIRE(node.stopsProjection());
    }

    SECTION("can be compared for equality")
    {
        ExpressionNode leftExpression{IdentifierNode{"id1"}};
        ExpressionNode rightExpression{IdentifierNode{"id2"}};
        OrExpressionNode node1{
            leftExpression,
            rightExpression};
        OrExpressionNode node2;
        node2 = node1;

        REQUIRE(node1 == node2);
        REQUIRE(node1 == node1);
    }

    SECTION("accepts visitor")
    {
        OrExpressionNode node;
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const OrExpressionNode*)))
                .AlwaysReturn();

        node.accept(&visitor.get());

        Verify(OverloadedMethod(visitor, visit, void(const OrExpressionNode*)))
                .Once();
    }
}
