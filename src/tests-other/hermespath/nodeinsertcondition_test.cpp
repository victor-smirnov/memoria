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
#include "parser/nodeinsertcondition.h"

#include <catch2/catch.hpp>

TEST_CASE("NodeInsertCondition")
{
    using namespace memoria::hermes::path::parser;
    namespace ast = memoria::hermes::path::ast;
    using namespace fakeit;

    NodeInsertCondition condition;

    SECTION("Returns false for terminal nodes")
    {
        ast::ExpressionNode targetNode{
            ast::IdentifierNode{}};
        ast::RawStringNode node;

        REQUIRE_FALSE(condition(targetNode, node));
    }

    SECTION("Returns false for subexpression and a terminal node")
    {
        ast::ExpressionNode targetNode{
            ast::SubexpressionNode{}};
        ast::IdentifierNode node;

        REQUIRE_FALSE(condition(targetNode, node));
    }

    SECTION("Returns true for an empty node and a terminal node")
    {
        ast::ExpressionNode targetNode{};
        ast::IdentifierNode node;

        REQUIRE(condition(targetNode, node));
    }

    SECTION("Returns true for a terminal node and a subexpression")
    {
        ast::ExpressionNode targetNode{
            ast::IdentifierNode{}};
        ast::SubexpressionNode node;

        REQUIRE(condition(targetNode, node));
    }

    SECTION("Returns false for two subexpression")
    {
        ast::ExpressionNode targetNode{
            ast::SubexpressionNode{}};
        ast::SubexpressionNode node;

        REQUIRE_FALSE(condition(targetNode, node));
    }

    SECTION("Returns false for a subexpression and array item")
    {
        ast::ExpressionNode targetNode{
            ast::SubexpressionNode{}};
        ast::IndexExpressionNode node{
            ast::BracketSpecifierNode{
                ast::ArrayItemNode{}}};

        REQUIRE_FALSE(condition(targetNode, node));
    }

    SECTION("Returns false for two flatten operators")
    {
        ast::ExpressionNode targetNode{
            ast::IndexExpressionNode{
                ast::BracketSpecifierNode{
                    ast::FlattenOperatorNode{}}}};
        ast::IndexExpressionNode node{
            ast::BracketSpecifierNode{
                ast::FlattenOperatorNode{}}};

        REQUIRE_FALSE(condition(targetNode, node));
    }

    SECTION("Returns true for two list wildcards")
    {
        ast::ExpressionNode targetNode{
            ast::IndexExpressionNode{
                ast::BracketSpecifierNode{
                    ast::ListWildcardNode{}}}};
        ast::IndexExpressionNode node{
            ast::BracketSpecifierNode{
                ast::ListWildcardNode{}}};

        REQUIRE(condition(targetNode, node));
    }

    SECTION("Returns true for two hash wildcards")
    {
        ast::ExpressionNode targetNode{
            ast::HashWildcardNode{}};
        ast::HashWildcardNode node{};

        REQUIRE(condition(targetNode, node));
    }

    SECTION("Returns true for a list wildcard and flatten operator")
    {
        ast::ExpressionNode targetNode{
            ast::IndexExpressionNode{
                ast::BracketSpecifierNode{
                    ast::ListWildcardNode{}}}};
        ast::IndexExpressionNode node{
            ast::BracketSpecifierNode{
                ast::FlattenOperatorNode{}}};

        REQUIRE(condition(targetNode, node));
    }

    SECTION("Returns false for a flatten operator and a list wildcard")
    {
        ast::ExpressionNode targetNode{
            ast::IndexExpressionNode{
                ast::BracketSpecifierNode{
                    ast::FlattenOperatorNode{}}}};
        ast::IndexExpressionNode node{
            ast::BracketSpecifierNode{
                ast::ListWildcardNode{}}};

        REQUIRE_FALSE(condition(targetNode, node));
    }
}
