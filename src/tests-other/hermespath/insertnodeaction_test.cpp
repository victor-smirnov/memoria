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
#include "parser/insertnodeaction.h"
#include "ast/allnodes.h"

#include <catch2/catch.hpp>

using ParameterPair = std::pair<memoria::hermes::path::ast::ExpressionNode,
                                memoria::hermes::path::ast::ExpressionNode>;
using ParameterPairs = std::vector<ParameterPair>;

class NodeInserterStub
{
public:
    using result_type = void;
    static ParameterPairs calls;

    result_type operator()(memoria::hermes::path::ast::ExpressionNode& target,
                           memoria::hermes::path::ast::ExpressionNode& node) const
    {
        calls.push_back({target, node});
    }
};
ParameterPairs NodeInserterStub::calls = ParameterPairs{};

class NodeInsertConditionStub
{
public:
    using result_type = bool;
    static ParameterPairs calls;
    static memoria::hermes::path::ast::ExpressionNode trueConditionNode;

    result_type operator()(const memoria::hermes::path::ast::ExpressionNode& target,
                           const memoria::hermes::path::ast::ExpressionNode& node) const
    {
        calls.push_back({target, node});
        return target == trueConditionNode;
    }
};
ParameterPairs NodeInsertConditionStub::calls = ParameterPairs{};
memoria::hermes::path::ast::ExpressionNode NodeInsertConditionStub::trueConditionNode
    = memoria::hermes::path::ast::ExpressionNode{};

TEST_CASE("InsertNodeAction")
{
    using namespace memoria::hermes::path::parser;
    namespace ast = memoria::hermes::path::ast;
    using namespace fakeit;

    NodeInserterStub::calls.clear();
    NodeInsertConditionStub::calls.clear();
    NodeInsertConditionStub::trueConditionNode
            = ast::ExpressionNode{ast::IdentifierNode{"true"}};
    InsertNodeAction<NodeInserterStub, NodeInsertConditionStub> insertAction;

    SECTION("calls insert condition, and if it returns false then it doesn't "
            "calls the inserter")
    {
        ast::ExpressionNode targetNode{
            ast::IdentifierNode{"target"}};
        ast::ExpressionNode node{
            ast::IdentifierNode{"node"}};
        ParameterPairs conditionCalls = {{targetNode, node}};

        insertAction(targetNode, node);

        REQUIRE(NodeInserterStub::calls.empty());
        REQUIRE(NodeInsertConditionStub::calls == conditionCalls);
    }

    SECTION("calls insert condition for all expression nodes on the left edge "
            "and if it returns false for all of them then it doesn't "
            "calls the inserter")
    {
        ast::ExpressionNode expressionNode{
            ast::IdentifierNode{"target"}};
        ast::ExpressionNode targetNode{
            ast::NotExpressionNode{expressionNode}};
        ast::ExpressionNode node{
            ast::IdentifierNode{"node"}};
        ParameterPairs conditionCalls = {{targetNode, node},
                                         {expressionNode, node}};

        insertAction(targetNode, node);

        REQUIRE(NodeInserterStub::calls.empty());
        REQUIRE(NodeInsertConditionStub::calls == conditionCalls);
    }

    SECTION("calls inserter for the node where insert condition returns true")
    {
        ast::ExpressionNode expressionNode{
            ast::IdentifierNode{"true"}};
        ast::ExpressionNode targetNode{
            ast::NotExpressionNode{expressionNode}};
        ast::ExpressionNode node{
            ast::IdentifierNode{"node"}};
        ParameterPairs conditionCalls = {{targetNode, node},
                                         {expressionNode, node}};
        ParameterPairs inserterCalls = {{expressionNode, node}};

        insertAction(targetNode, node);

        REQUIRE(NodeInserterStub::calls == inserterCalls);
        REQUIRE(NodeInsertConditionStub::calls == conditionCalls);
    }
}
