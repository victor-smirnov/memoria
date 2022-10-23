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

TEST_CASE("BracketSpecifierNode")
{
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;

    SECTION("can be constructed")
    {
        SECTION("without arguments")
        {
            REQUIRE_NOTHROW(BracketSpecifierNode{});
        }

        SECTION("with array item")
        {
            ArrayItemNode arrayItem{3};

            BracketSpecifierNode node{arrayItem};

            REQUIRE(node == arrayItem);
        }

        SECTION("with flatten operator")
        {
            FlattenOperatorNode flattenNode;

            BracketSpecifierNode node{flattenNode};

            REQUIRE(node == flattenNode);
        }

        SECTION("with slice expression")
        {
            SliceExpressionNode sliceNode;

            BracketSpecifierNode node{sliceNode};

            REQUIRE(node == sliceNode);
        }

        SECTION("with list wildcard expression")
        {
            ListWildcardNode listWildcardNode;

            BracketSpecifierNode node{listWildcardNode};

            REQUIRE(node == listWildcardNode);
        }

        SECTION("with filter expression")
        {
            FilterExpressionNode filterNode;

            BracketSpecifierNode node{filterNode};

            REQUIRE(node == filterNode);
        }
    }

    SECTION("can be compared for equality")
    {
        ArrayItemNode arrayItem{3};
        BracketSpecifierNode node1{arrayItem};
        BracketSpecifierNode node2;
        node2 = node1;

        REQUIRE(node1 == node2);
        REQUIRE(node1 == node1);
    }

    SECTION("accepts visitor")
    {
        BracketSpecifierNode node{ArrayItemNode{}};
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const ArrayItemNode*)))
                .AlwaysReturn();

        node.accept(&visitor.get());

        Verify(OverloadedMethod(visitor, visit, void(const ArrayItemNode*)))
                .Once();
    }

    SECTION("returns true for isProjected if actual expression should be "
            "projected")
    {
        BracketSpecifierNode node{FlattenOperatorNode{}};

        REQUIRE(node.isProjection());
    }

    SECTION("returns false for isProjected if actual expression should not be "
            "projected")
    {
        BracketSpecifierNode node{ArrayItemNode{}};

        REQUIRE_FALSE(node.isProjection());
    }

    SECTION("returns false for isProjected if node is empty")
    {
        BracketSpecifierNode node;

        REQUIRE_FALSE(node.isProjection());
    }

    SECTION("returns false for stopsProjection if node is empty")
    {
        BracketSpecifierNode node;

        REQUIRE_FALSE(node.stopsProjection());
    }

    SECTION("returns false for stopsProjection if actual expression doesn't"
            "stops a projection")
    {
        BracketSpecifierNode node{ArrayItemNode{}};

        REQUIRE_FALSE(node.stopsProjection());
    }

    SECTION("returns true for stopsProjection if actual expression stops a"
            "projections")
    {
        BracketSpecifierNode node{FlattenOperatorNode{}};

        REQUIRE(node.stopsProjection());
    }
}
