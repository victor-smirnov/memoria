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
#include "parser/leftedgeiterator.h"
#include "ast/allnodes.h"

#include <catch2/catch.hpp>

TEST_CASE("LeftEdgeIterator")
{
    using namespace memoria::hermes::path::parser;
    namespace ast = memoria::hermes::path::ast;
    using namespace fakeit;

    SECTION("Can be constructed")
    {
        SECTION("without parameters")
        {
            REQUIRE_NOTHROW(LeftEdgeIterator{});
        }

        SECTION("with node parameter")
        {
            ast::ExpressionNode node;

            REQUIRE_NOTHROW(LeftEdgeIterator{node});
        }
    }

    SECTION("When default constructed")
    {
        LeftEdgeIterator it;

        SECTION("dereference operator returns nullptr")
        {
            REQUIRE(it.operator->() == nullptr);
        }

        SECTION("doesn't' equals to iterator constructed with a node")
        {
            ast::ExpressionNode node;
            LeftEdgeIterator it2{node};

            REQUIRE(it2 != it);
        }

        SECTION("equals to another default constructed iterator")
        {
            LeftEdgeIterator it2;

            REQUIRE_FALSE(it2 != it);
        }
    }

    SECTION("Constructed with node parameter")
    {
        ast::ExpressionNode childNode{
            ast::IdentifierNode{"id"}};
        ast::ExpressionNode node{
            ast::NotExpressionNode{childNode}};
        LeftEdgeIterator it{node};

        SECTION("dereference operator returns pointer to node")
        {
            REQUIRE(it.operator->() == &node);
        }

        SECTION("indirection operator returns reference to node")
        {
            REQUIRE(*it == node);
        }

        SECTION("is not equal to iterator constructed with a different node")
        {
            ast::ExpressionNode node2;
            LeftEdgeIterator it2{node2};

            REQUIRE(it != it2);
        }

        SECTION("is equal to iterator constructed with the same node")
        {
            LeftEdgeIterator it2{node};

            REQUIRE_FALSE(it != it2);
        }

        SECTION("moves to child node on preincrement")
        {
            ++it;

            REQUIRE(*it == childNode);
        }

        SECTION("moves to child node and returns iterator to parent node on "
                "postincrement")
        {
            auto it2 = it++;

            REQUIRE(*it == childNode);
            REQUIRE(*it2 == node);
        }
    }
}
