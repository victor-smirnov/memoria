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
#include <memoria/core/hermes/path/types.h>
#include "parser/leftchildextractor.h"
#include "ast/allnodes.h"

#include <catch2/catch.hpp>

TEST_CASE("LeftChildExtractor")
{
    using namespace memoria::hermes::path::parser;
    namespace ast = memoria::hermes::path::ast;
    using namespace fakeit;

    LeftChildExtractor policy;

    SECTION("Returns left child of binary expression node")
    {
        ast::ExpressionNode node{
            ast::SubexpressionNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id1"}},
                ast::ExpressionNode{
                    ast::IdentifierNode{"id2"}}}};

        REQUIRE(policy(&node)
                == &(boost::get<ast::SubexpressionNode>(
                    &node.value)->leftExpression));
    }

    SECTION("Returns nullptr for non binary expression node")
    {
        ast::ExpressionNode node{
            ast::IdentifierNode{}};

        REQUIRE(policy(&node) == nullptr);
    }

    SECTION("Returns nullptr for empty expression")
    {
        ast::ExpressionNode node{};

        REQUIRE(policy(&node) == nullptr);
    }
}
