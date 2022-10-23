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
#include "parser/noderank.h"
#include "ast/allnodes.h"

#include <catch2/catch.hpp>

TEST_CASE("nodeRank")
{
    using namespace memoria::hermes::path::parser;
    using namespace memoria::hermes::path::ast;
    using namespace fakeit;

    SECTION("ranks basic nodes at 0")
    {
        REQUIRE(nodeRank(IdentifierNode{}) == 0);
        REQUIRE(nodeRank(RawStringNode{}) == 0);
        REQUIRE(nodeRank(LiteralNode{}) == 0);
    }

    SECTION("ranks empty expression node at -1")
    {
        REQUIRE(nodeRank(ExpressionNode{}) == -1);
    }

    SECTION("ranks non empty expression node with contained expression rank")
    {
        IdentifierNode containedNode;
        ExpressionNode node{containedNode};

        REQUIRE(nodeRank(node) == nodeRank(containedNode));
    }

    SECTION("ranks subexpression node at 1")
    {
        REQUIRE(nodeRank(SubexpressionNode{}) == 1);
    }

    SECTION("ranks array item node at 1")
    {
        REQUIRE(nodeRank(ArrayItemNode{}) == 1);
    }

    SECTION("ranks flatten operator node at 2")
    {
        REQUIRE(nodeRank(FlattenOperatorNode{}) == 2);
    }

    SECTION("ranks slice expression node at 2")
    {
        REQUIRE(nodeRank(SliceExpressionNode{}) == 2);
    }

    SECTION("ranks list wildcard expression node at 2")
    {
        REQUIRE(nodeRank(ListWildcardNode{}) == 2);
    }

    SECTION("ranks hash wildcard expression node at 2")
    {
        REQUIRE(nodeRank(HashWildcardNode{}) == 2);
    }

    SECTION("ranks filter expression node at 2")
    {
        REQUIRE(nodeRank(FilterExpressionNode{}) == 2);
    }

    SECTION("ranks not expression node at 3")
    {
        REQUIRE(nodeRank(NotExpressionNode{}) == 3);
    }

    SECTION("ranks comparator expression node at 4")
    {
        REQUIRE(nodeRank(ComparatorExpressionNode{}) == 4);
    }    

    SECTION("ranks and expression node at 5")
    {
        REQUIRE(nodeRank(AndExpressionNode{}) == 5);
    }

    SECTION("ranks or expression node at 6")
    {
        REQUIRE(nodeRank(OrExpressionNode{}) == 6);
    }

    SECTION("ranks pipe expression node at 7")
    {
        REQUIRE(nodeRank(PipeExpressionNode{}) == 7);
    }

    SECTION("ranks bracket specifier node as its expression")
    {
        ArrayItemNode expression1;
        BracketSpecifierNode node1{expression1};
        FlattenOperatorNode expression2;
        BracketSpecifierNode node2{expression2};

        REQUIRE(nodeRank(node1) == nodeRank(expression1));
        REQUIRE(nodeRank(node2) == nodeRank(expression2));
    }

    SECTION("ranks index expression node as its bracket specifier")
    {
        BracketSpecifierNode bracketSpecifier1{
            ArrayItemNode{}};
        IndexExpressionNode node1{bracketSpecifier1};
        BracketSpecifierNode bracketSpecifier2{
            FlattenOperatorNode{}};
        IndexExpressionNode node2{bracketSpecifier2};

        REQUIRE(nodeRank(node1) == nodeRank(bracketSpecifier1));
        REQUIRE(nodeRank(node2) == nodeRank(bracketSpecifier2));
    }
}
