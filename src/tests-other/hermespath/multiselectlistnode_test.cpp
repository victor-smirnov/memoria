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

TEST_CASE("MultiselectListNode")
{
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;

    SECTION("can be constructed")
    {
        SECTION("without parameters")
        {
            REQUIRE_NOTHROW(MultiselectListNode{});
        }

        SECTION("with vector of expressions")
        {
            std::vector<ExpressionNode> expressions = {
                ExpressionNode{
                    IdentifierNode{"id1"}},
                ExpressionNode{
                    IdentifierNode{"id2"}},
                ExpressionNode{
                    IdentifierNode{"id3"}}};

            MultiselectListNode node{expressions};

            REQUIRE(node.expressions == expressions);
        }

        SECTION("with initializer list of expressions")
        {
            ExpressionNode node1{
                IdentifierNode{"id1"}};
            ExpressionNode node2{
                IdentifierNode{"id2"}};
            ExpressionNode node3{
                IdentifierNode{"id3"}};
            std::vector<ExpressionNode> expressions{node1, node2, node3};

            MultiselectListNode node{node1, node2, node3};

            REQUIRE(node.expressions == expressions);
        }
    }

    SECTION("can be compared for equality")
    {
        ExpressionNode exp1{
            IdentifierNode{"id1"}};
        ExpressionNode exp2{
            IdentifierNode{"id2"}};
        ExpressionNode exp3{
            IdentifierNode{"id3"}};
        MultiselectListNode node1{exp1, exp2, exp3};
        MultiselectListNode node2;
        node2 = node1;

        REQUIRE(node1 == node2);
        REQUIRE(node1 == node1);
    }

    SECTION("accepts visitor")
    {
        MultiselectListNode node;
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const MultiselectListNode*)))
                .AlwaysReturn();

        node.accept(&visitor.get());

        Verify(OverloadedMethod(visitor,
                                visit,
                                void(const MultiselectListNode*)))
                .Once();
    }
}
