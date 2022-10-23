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
#include "ast/sliceexpressionnode.h"
#include <boost/optional/optional_io.hpp>

#include <catch2/catch.hpp>

TEST_CASE("SliceExpressionNode")
{
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;
    using memoria::hermes::path::Index;

    SECTION("can be constructed")
    {
        SECTION("without parameters")
        {
            SliceExpressionNode node{};

            REQUIRE_FALSE(node.start);
            REQUIRE_FALSE(node.stop);
            REQUIRE_FALSE(node.step);
        }

        SECTION("with start index")
        {
            SliceExpressionNode node{SliceExpressionNode::IndexType{3}};

            REQUIRE(node.start == Index{3});
            REQUIRE_FALSE(node.stop);
            REQUIRE_FALSE(node.step);
        }

        SECTION("with start and stop index")
        {
            SliceExpressionNode node{Index{3}, Index{5}};

            REQUIRE(node.start == Index{3});
            REQUIRE(node.stop == Index{5});
            REQUIRE_FALSE(node.step);
        }

        SECTION("with start, stop and step index")
        {
            SliceExpressionNode node{Index{3}, Index{5}, Index{-1}};

            REQUIRE(node.start == Index{3});
            REQUIRE(node.stop == Index{5});
            REQUIRE(node.step == Index{-1});
        }
    }

    SECTION("can be compared for equality")
    {
        SliceExpressionNode node1{Index{3}, Index{5}, Index{-1}};
        SliceExpressionNode node2;
        node2 = node1;

        REQUIRE(node1 == node2);
        REQUIRE(node1 == node1);
    }

    SECTION("accepts visitor")
    {
        SliceExpressionNode node{};
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const SliceExpressionNode*)))
                .AlwaysReturn();

        node.accept(&visitor.get());

        Verify(OverloadedMethod(visitor,
                                visit,
                                void(const SliceExpressionNode*)))
                .Once();
    }
}
