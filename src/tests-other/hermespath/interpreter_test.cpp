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
#include "interpreter/interpreter.h"
#include "ast/allnodes.h"
#include <memoria/core/hermes/path/exceptions.h>
#include <fstream>
#include <chrono>
#include <boost/range/algorithm.hpp>

#include <catch2/catch.hpp>

//namespace memoria::hermes::path { namespace ast {

//std::ostream& operator<< (std::ostream& stream, ExpressionNode const&)
//{
//    return stream;
//}
//}}

TEST_CASE("ContextValue")
{
    using namespace memoria::hermes::path;
    //using memoria::hermes::path::interpreter::JsonRef;
    using memoria::hermes::path::interpreter::ContextValue;

    SECTION("can be constructed with Json lvalue")
    {
        Json jsonValue {{"key", "value"}};

        ContextValue value {jsonValue};

        REQUIRE(value.which() == 0);
        REQUIRE(boost::get<const Json&>(value) == jsonValue);
    }

    SECTION("can be constructed with Json rvalue")
    {
        ContextValue value {Json{{"key", "value"}}};

        REQUIRE(value.which() == 0);
        REQUIRE(boost::get<const Json&>(value) == Json{{"key", "value"}});
    }

    SECTION("can be constructed with Json reference wrapper")
    {
        Json jsonValue {{"key", "value"}};

        ContextValue value {std::cref(jsonValue)};

        REQUIRE(value.which() == 1);
        REQUIRE(boost::get<const JsonRef&>(value) == jsonValue);
    }

    SECTION("can assing Json lvalue")
    {
        Json jsonValue {{"key", "value"}};
        ContextValue value;

        value = jsonValue;

        REQUIRE(value.which() == 0);
        REQUIRE(boost::get<const Json&>(value) == jsonValue);
    }


    SECTION("can assing Json rvalue")
    {
        ContextValue value {Json{{"key", "value"}}};

        REQUIRE(value.which() == 0);
        REQUIRE(boost::get<const Json&>(value) == Json{{"key", "value"}});
    }

    SECTION("can asign Json reference wrapper")
    {
        Json jsonValue {{"key", "value"}};
        ContextValue value;

        value = std::cref(jsonValue);

        REQUIRE(value.which() == 1);
        REQUIRE(boost::get<const JsonRef&>(value) == jsonValue);
    }
}

TEST_CASE("assignContextValue")
{
    using namespace memoria::hermes::path;
    //using memoria::hermes::path::interpreter::JsonRef;
    using memoria::hermes::path::interpreter::ContextValue;
    using memoria::hermes::path::interpreter::assignContextValue;

    SECTION("moves Json rvalue")
    {
        Json jsonValue {{"key", "value"}};
        Json expectedValue(jsonValue);
        REQUIRE(jsonValue.type() == Json::value_t::object);
        REQUIRE(expectedValue == jsonValue);
        ContextValue contextValue;

        contextValue = assignContextValue(std::move(jsonValue));

        REQUIRE(contextValue.which() == 0);
        REQUIRE(boost::get<const Json&>(contextValue) == expectedValue);
        REQUIRE(jsonValue == Json{});
    }

    SECTION("returns reference_wrapper for Json lvalue ref")
    {
        Json jsonValue {{"key", "value"}};
        ContextValue contextValue;

        contextValue = assignContextValue(jsonValue);

        REQUIRE(contextValue.which() == 1);
        REQUIRE(boost::get<const JsonRef&>(contextValue) == jsonValue);
        REQUIRE_FALSE(jsonValue == Json{});
    }
}

TEST_CASE("getJsonValue")
{
    using namespace memoria::hermes::path;
    using memoria::hermes::path::interpreter::JsonRef;
    using memoria::hermes::path::interpreter::ContextValue;
    using memoria::hermes::path::interpreter::getJsonValue;
    Json jsonValue {{"key", "value"}};

    SECTION("returns const ref for Json value context")
    {
        ContextValue context {jsonValue};

        const Json& result = getJsonValue(context);

        REQUIRE(result == jsonValue);
    }

    SECTION("returns const ref for Json reference_wrapper context")
    {
        ContextValue context {std::cref(jsonValue)};

        const Json& result = getJsonValue(context);

        REQUIRE(result == jsonValue);
    }
}

TEST_CASE("Interpreter")
{
    using namespace memoria::hermes::path;
    using memoria::hermes::path::interpreter::Interpreter;
    using memoria::hermes::path::interpreter::assignContextValue;
    using memoria::hermes::path::interpreter::getJsonValue;
    namespace ast = memoria::hermes::path::ast;
    namespace rng = boost::range;
    using namespace fakeit;

    Interpreter interpreter;

    SECTION("can set context with Json lvalue ref")
    {
        auto context = "\"value\""_json;
        interpreter.setContext(context);

        REQUIRE(interpreter.currentContext() == context);
        REQUIRE(interpreter.currentContextValue().which() == 1);
    }

    SECTION("can set context with Json rvalue")
    {
        auto context = "\"value\""_json;
        auto expected = Json(context);
        interpreter.setContext(std::move(context));

        REQUIRE(interpreter.currentContext() == expected);
        REQUIRE(interpreter.currentContextValue().which() == 0);
    }

    SECTION("accepts abstract node")
    {
        Mock<ast::AbstractNode> node;
        When(Method(node, accept).Using(&interpreter)).AlwaysReturn();

        interpreter.visit(&node.get());

        Verify(Method(node, accept)).Once();
    }

    SECTION("accepts expression node")
    {
        Mock<ast::ExpressionNode> node;
        When(Method(node, accept).Using(&interpreter)).AlwaysReturn();

        interpreter.visit(&node.get());

        Verify(Method(node, accept)).Once();
    }

    SECTION("evaluates identifier node with lvalue ref")
    {
        ast::IdentifierNode node{"identifier"};
        int value = 15;
        Json expectedValue = value;
        REQUIRE(expectedValue.is_number());
        Json jsonValue{{"identifier", value}};
        interpreter.setContext(jsonValue);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 1);
        REQUIRE(interpreter.currentContext() == expectedValue);
    }

    SECTION("evaluates identifier node with rvalue")
    {
        ast::IdentifierNode node{"identifier"};
        int value = 15;
        Json expectedValue = value;
        REQUIRE(expectedValue.is_number());
        interpreter.setContext(Json{{"identifier", value}});

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == expectedValue);
    }

    SECTION("evaluates non existing identifier to null")
    {
        ast::IdentifierNode node{"non-existing"};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates non existing identifier in an lvalue ref object to null")
    {
        Json jsonValue{{"identifier", "value"}};
        interpreter.setContext(jsonValue);
        ast::IdentifierNode node{"non-existing"};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates non existing identifier in an rvalue ref object to null")
    {
        Json jsonValue{{"identifier", "value"}};
        interpreter.setContext(Json{{"identifier", "value"}});
        ast::IdentifierNode node{"non-existing"};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates identifier on non object to null")
    {
        ast::IdentifierNode node{"identifier"};
        interpreter.setContext(Json{15});

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates raw string")
    {
        String rawString{"[baz]"};
        ast::RawStringNode node{rawString};
        Json expectedValue = rawString;
        REQUIRE(expectedValue.is_string());

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()  == expectedValue);
    }

    SECTION("evaluates literal")
    {
        String stringLiteralValue{"foo"};
        ast::LiteralNode stringNode{"\"" + stringLiteralValue + "\""};
        ast::LiteralNode arrayNode{"[1, 2, 3]"};
        Json expectedStringValue = stringLiteralValue;
        Json expectedArrayValue{1, 2, 3};
        REQUIRE(expectedStringValue.is_string());
        REQUIRE(expectedArrayValue.is_array());

        interpreter.visit(&stringNode);
        auto stringResult = interpreter.currentContext();
        interpreter.visit(&arrayNode);
        auto arrayResult = interpreter.currentContext();

        REQUIRE(stringResult == expectedStringValue);
        REQUIRE(arrayResult == expectedArrayValue);
    }

    SECTION("evaluates subexpression")
    {
        Mock<ast::SubexpressionNode> node;
        When(Method(node, accept)).AlwaysReturn();

        interpreter.visit(&node.get());

        Verify(Method(node, accept).Using(&interpreter)).Once();
    }

    SECTION("evaluates bracket specifier")
    {
        Mock<ast::BracketSpecifierNode> node;
        When(Method(node, accept)).AlwaysReturn();

        interpreter.visit(&node.get());

        Verify(Method(node, accept).Using(&interpreter)).Once();
    }

    SECTION("evaluates index expression")
    {
        ast::IndexExpressionNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        interpreterMock.get().setContext("[1, 2, 3]"_json);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::BracketSpecifierNode*)))
                .AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)
               + OverloadedMethod(interpreterMock, visit,
                                  void(const ast::BracketSpecifierNode*))
                    .Using(&node.bracketSpecifier))
                .Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates index expression on non array to null without evaluating"
            "bracket specifier")
    {
        ast::IndexExpressionNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        interpreterMock.get().setContext("\"string value\""_json);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();

        interpreterMock.get().visit(&node);

        REQUIRE(interpreterMock.get().currentContext() == Json{});
        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates projected index expression")
    {
        ast::IndexExpressionNode node{
            ast::ExpressionNode{},
            ast::BracketSpecifierNode{
                ast::FlattenOperatorNode{}},
            ast::ExpressionNode{}};
        Mock<Interpreter> interpreterMock(interpreter);
        interpreterMock.get().setContext("[1, 2, 3]"_json);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::BracketSpecifierNode*)))
                .AlwaysReturn();
        When(OverloadedMethod(interpreterMock, evaluateProjection,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)
               + OverloadedMethod(interpreterMock, visit,
                                  void(const ast::BracketSpecifierNode*))
                    .Using(&node.bracketSpecifier)
               + OverloadedMethod(interpreterMock, evaluateProjection,
                                  void(const ast::ExpressionNode*))
                    .Using(&node.rightExpression))
                .Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates array item expression with lvalue ref")
    {
        ast::ArrayItemNode node{2};
        Json value {"zero", "one", "two", "three", "four"};
        interpreter.setContext(value);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 1);
        REQUIRE(interpreter.currentContext() == "two");
    }

    SECTION("evaluates array item expression with rvalue")
    {
        ast::ArrayItemNode node{2};
        interpreter.setContext(Json{"zero", "one", "two", "three", "four"});

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == "two");
    }

    SECTION("evaluates array item expression with negative index lvalue ref")
    {
        ast::ArrayItemNode node{-4};
        Json value {"zero", "one", "two", "three", "four"};
        interpreter.setContext(value);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 1);
        REQUIRE(interpreter.currentContext() == "one");
    }

    SECTION("evaluates array item expression with negative index")
    {
        ast::ArrayItemNode node{-4};
        interpreter.setContext(Json{"zero", "one", "two", "three", "four"});

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == "one");
    }

    SECTION("evaluates array item expression on non arrays to null")
    {
        ast::ArrayItemNode node{2};
        interpreter.setContext(Json{3});

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates array item expression with out of bounds index to null")
    {
        ast::ArrayItemNode node{15};
        interpreter.setContext(Json{"zero", "one", "two", "three", "four"});

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates projection on lvalue ref")
    {
        Json context = {{{"id", 1}}, {{"id", 2}}, {{"id2", 3}}, {{"id", 4}}};
        REQUIRE(context.is_array());
        ast::ExpressionNode expression{
            ast::IdentifierNode{"id"}};
        interpreter.setContext(context);
        Json expectedResult = {1, 2, 4};
        REQUIRE(expectedResult.is_array());

        interpreter.evaluateProjection(&expression);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates projection on rvalue")
    {
        Json context = {{{"id", 1}}, {{"id", 2}}, {{"id2", 3}}, {{"id", 4}}};
        REQUIRE(context.is_array());
        ast::ExpressionNode expression{
            ast::IdentifierNode{"id"}};
        interpreter.setContext(std::move(context));
        Json expectedResult = {1, 2, 4};
        REQUIRE(expectedResult.is_array());

        interpreter.evaluateProjection(&expression);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates projection on non arrays to null")
    {
        Json context = "string";
        REQUIRE(context.is_string());
        ast::ExpressionNode expression{
            ast::IdentifierNode{"id"}};
        interpreter.setContext(context);

        interpreter.evaluateProjection(&expression);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates flatten operator on lvalue ref")
    {
        Json context = "[1, 2, [3], [4, [5, 6, 7], 8], [9, 10] ]"_json;
        Json contextCopy(context);
        interpreter.setContext(context);
        Json expectedResult = "[1, 2, 3, 4, [5, 6, 7], 8, 9, 10]"_json;
        ast::FlattenOperatorNode flattenNode;

        interpreter.visit(&flattenNode);

        REQUIRE(interpreter.currentContext() == expectedResult);
        REQUIRE(context == contextCopy);
    }

    SECTION("evaluates flatten operator on rvalue")
    {
        Json context = "[1, 2, [3], [4, [5, 6, 7], 8], [9, 10] ]"_json;
        Json contextCopy{context};
        interpreter.setContext(std::move(context));
        Json expectedResult = "[1, 2, 3, 4, [5, 6, 7], 8, 9, 10]"_json;
        ast::FlattenOperatorNode flattenNode;

        interpreter.visit(&flattenNode);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates flatten operator on non array to null")
    {
        Json context = {{"id", "value"}};
        REQUIRE(context.is_object());
        interpreter.setContext(context);
        ast::FlattenOperatorNode flattenNode;

        interpreter.visit(&flattenNode);

        REQUIRE(interpreter.currentContext() == Json{});
    }


    SECTION("evaluates slice expression on non array to null")
    {
        ast::SliceExpressionNode sliceNode{Index{2}, Index{5}};

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("throws InvalidValue exception when slice expression"
            "step equals to zero")
    {
        interpreter.setContext("[]"_json);
        ast::SliceExpressionNode sliceNode{Index{2}, Index{5}, Index{0}};

        REQUIRE_THROWS_AS(interpreter.visit(&sliceNode), InvalidValue);
    }

    SECTION("evaluates slice expression on lvalue ref")
    {
        Json context = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"_json;
        interpreter.setContext(context);
        ast::SliceExpressionNode sliceNode{Index{2}, Index{5}};
        Json expectedResult = "[2, 3, 4]"_json;

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates slice expression on rvalue")
    {
        Json context = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"_json;
        interpreter.setContext(std::move(context));
        ast::SliceExpressionNode sliceNode{Index{2}, Index{5}};
        Json expectedResult = "[2, 3, 4]"_json;

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates slice expression with step index")
    {
        Json context = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"_json;
        interpreter.setContext(context);
        ast::SliceExpressionNode sliceNode{Index{2}, Index{5}, Index{2}};
        Json expectedResult = "[2, 4]"_json;

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates slice expression with negative step index")
    {
        Json context = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"_json;
        interpreter.setContext(context);
        ast::SliceExpressionNode sliceNode{Index{5}, Index{2}, Index{-1}};
        Json expectedResult = "[5, 4, 3]"_json;

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates empty slice expression")
    {
        Json context = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"_json;
        interpreter.setContext(context);
        ast::SliceExpressionNode sliceNode;

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == context);
    }

    SECTION("evaluates slice expression with end value over the end of array")
    {
        Json context = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"_json;
        interpreter.setContext(context);
        ast::SliceExpressionNode sliceNode{Index{0}, Index{20}};

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == context);
    }

    SECTION("evaluates slice expression with start value below the first item"
            "of array")
    {
        Json context = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"_json;
        interpreter.setContext(context);
        ast::SliceExpressionNode sliceNode{
            ast::SliceExpressionNode::IndexType{-50}};

        interpreter.visit(&sliceNode);

        REQUIRE(interpreter.currentContext() == context);
    }

    SECTION("evaluates list wildcard expression on non array to null")
    {
        Json context = "string";
        interpreter.setContext(context);
        ast::ListWildcardNode node;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates list wildcard expression on arrays to the array itself")
    {
        Json context = "[1, 2, 3]"_json;
        interpreter.setContext(context);
        ast::ListWildcardNode node;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == context);
    }

    SECTION("evaluates hash wildcard expression on non object to null")
    {
        Json context = "[1, 2, 3]"_json;
        interpreter.setContext(context);
        ast::HashWildcardNode node;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates hash wildcard expression on objects to array of values "
            "from lvalue ref")
    {
        Json context = "{\"a\": 1, \"b\":2, \"c\":3}"_json;
        Json values(Json::value_t::array);
        rng::copy(context, std::back_inserter(values));
        interpreter.setContext(context);
        ast::HashWildcardNode node;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == values);
    }

    SECTION("evaluates hash wildcard expression on objects to array of values "
            "from rvalue")
    {
        Json context = "{\"a\": 1, \"b\":2, \"c\":3}"_json;
        Json values(Json::value_t::array);
        rng::copy(context, std::back_inserter(values));
        interpreter.setContext(std::move(context));
        ast::HashWildcardNode node;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == values);
    }

    SECTION("evaluates left expression of hash wildcard expresion and projects"
            "right expression")
    {
        ast::HashWildcardNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();
        When(OverloadedMethod(interpreterMock, evaluateProjection,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)
               + OverloadedMethod(interpreterMock, evaluateProjection,
                                  void(const ast::ExpressionNode*))
                    .Using(&node.rightExpression))
                .Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("visits child expressions of multiselect list expression on "
            "evaluation")
    {
        ast::ExpressionNode exp1{
            ast::IdentifierNode{"id1"}};
        ast::ExpressionNode exp2{
            ast::IdentifierNode{"id2"}};
        ast::ExpressionNode exp3{
            ast::IdentifierNode{"id3"}};
        ast::MultiselectListNode node{exp1, exp2, exp3};
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();
        interpreterMock.get().setContext("\"value\""_json);

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.expressions[0])
                + OverloadedMethod(interpreterMock, visit,
                                   void(const ast::ExpressionNode*))
                       .Using(&node.expressions[1])
                + OverloadedMethod(interpreterMock, visit,
                                   void(const ast::ExpressionNode*))
                       .Using(&node.expressions[2]))
                .Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("doesn't evaluates multiselect list expression on null context")
    {
        ast::ExpressionNode exp1{
            ast::IdentifierNode{"id1"}};
        ast::ExpressionNode exp2{
            ast::IdentifierNode{"id2"}};
        ast::ExpressionNode exp3{
            ast::IdentifierNode{"id3"}};
        ast::MultiselectListNode node{exp1, exp2, exp3};
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();

        interpreterMock.get().visit(&node);

        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates multiselect list by creating a list from the child"
            "expressions' results")
    {
        interpreter.setContext("{\"id1\": 1, \"id2\":2, \"id3\":3}"_json);
        ast::ExpressionNode exp1{
            ast::IdentifierNode{"id1"}};
        ast::ExpressionNode exp2{
            ast::IdentifierNode{"id2"}};
        ast::ExpressionNode exp3{
            ast::IdentifierNode{"id3"}};
        ast::MultiselectListNode node{exp1, exp2, exp3};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2, 3]"_json);
    }

    SECTION("visits child expressions of multiselect hash expression on "
            "evaluation")
    {
        ast::MultiselectHashNode node{
                {ast::IdentifierNode{"id1"},
                 ast::ExpressionNode{
                    ast::IdentifierNode{"id2"}}},
                {ast::IdentifierNode{"id3"},
                 ast::ExpressionNode{
                    ast::IdentifierNode{"id4"}}}};
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();
        interpreterMock.get().setContext("\"value\""_json);

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
               .Using(&node.expressions[0].second)).Once();
        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
               .Using(&node.expressions[1].second)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("doesn't evaluates multiselect hash expression on null context")
    {
        ast::MultiselectHashNode node{
                {ast::IdentifierNode{"id1"},
                 ast::ExpressionNode{
                    ast::IdentifierNode{"id2"}}},
                {ast::IdentifierNode{"id3"},
                 ast::ExpressionNode{
                    ast::IdentifierNode{"id4"}}}};
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();

        interpreterMock.get().visit(&node);

        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates multiselect hash by creating an object from the child"
            "expressions' key and value")
    {
        interpreter.setContext("{\"id2\":\"value2\", \"id4\":\"value4\"}"_json);
        ast::MultiselectHashNode node{
                {ast::IdentifierNode{"id1"},
                 ast::ExpressionNode{
                    ast::IdentifierNode{"id2"}}},
                {ast::IdentifierNode{"id3"},
                 ast::ExpressionNode{
                    ast::IdentifierNode{"id4"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()
                == "{\"id1\":\"value2\",\"id3\":\"value4\"}"_json);
    }

    SECTION("evaluates child expression of not expression")
    {
        ast::NotExpressionNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
               .Using(&node.expression)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates not expression on false to true")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":false}"_json);
        Json expectedResult = true;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on true to false")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":true}"_json);
        Json expectedResult = false;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on null to true")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":null}"_json);
        Json expectedResult = true;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on all numbers to false")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        Json expectedResult = false;

        interpreter.setContext("{\"id\":5}"_json);
        interpreter.visit(&node);
        auto result1 = interpreter.currentContext();
        interpreter.setContext("{\"id\":0}"_json);
        interpreter.visit(&node);
        auto result2 = interpreter.currentContext();
        interpreter.setContext("{\"id\":-5}"_json);
        interpreter.visit(&node);
        auto result3 = interpreter.currentContext();

        REQUIRE(result1 == expectedResult);
        REQUIRE(result2 == expectedResult);
        REQUIRE(result3 == expectedResult);
    }

    SECTION("evaluates not expression on empty string to true")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":\"\"}"_json);
        Json expectedResult = true;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on non empty string to false")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":\"string\"}"_json);
        Json expectedResult = false;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on empty array to true")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":[]}"_json);
        Json expectedResult = true;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on non empty array to false")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":[1, 2, 3]}"_json);
        Json expectedResult = false;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on empty object to true")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":{}}"_json);
        Json expectedResult = true;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates not expression on non empty object to false")
    {
        ast::NotExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\":{\"id2\":\"string\"}}"_json);
        Json expectedResult = false;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates comparator expression")
    {
        ast::ComparatorExpressionNode node{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::NotEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)
               + OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.rightExpression)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates comparator expression with less operator")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::Less,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::Less,
            ast::ExpressionNode{
                ast::LiteralNode{"2"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();

        REQUIRE(result1);
        REQUIRE_FALSE(result2);
    }

    SECTION("evaluates comparator expression with less operator to "
            "null with non number")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"null"}},
            ast::ComparatorExpressionNode::Comparator::Less,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::Less,
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();

        REQUIRE(result1 == "null"_json);
        REQUIRE(result2 == "null"_json);
    }

    SECTION("evaluates comparator expression with less or equal operator")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::LessOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::LessOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"2"}}};
        ast::ComparatorExpressionNode node3{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::LessOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        Json result3 = interpreter.currentContext();

        REQUIRE(result1);
        REQUIRE_FALSE(result2);
        REQUIRE(result3);
    }

    SECTION("evaluates comparator expression with less or equal operator to "
            "null with non number")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"[]"}},
            ast::ComparatorExpressionNode::Comparator::LessOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::LessOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}};
        ast::ComparatorExpressionNode node3{
            ast::ExpressionNode{
                ast::LiteralNode{"null"}},
            ast::ComparatorExpressionNode::Comparator::LessOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        Json result3 = interpreter.currentContext();

        REQUIRE(result1 == "null"_json);
        REQUIRE(result2 == "null"_json);
        REQUIRE(result3 == "null"_json);
    }

    SECTION("evaluates comparator expression with equal operator")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::Equal,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::Equal,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();

        REQUIRE(result1);
        REQUIRE_FALSE(result2);
    }

    SECTION("evaluates comparator expression with greater or equal operator")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"2"}}};
        ast::ComparatorExpressionNode node3{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        Json result3 = interpreter.currentContext();

        REQUIRE_FALSE(result1);
        REQUIRE(result2);
        REQUIRE(result3);
    }

    SECTION("evaluates comparator expression with greater or equal operator "
            "to null with non number")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"false"}},
            ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"[]"}}};
        ast::ComparatorExpressionNode node3{
            ast::ExpressionNode{
                ast::LiteralNode{"null"}},
            ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        Json result3 = interpreter.currentContext();

        REQUIRE(result1 == "null"_json);
        REQUIRE(result2 == "null"_json);
        REQUIRE(result3 == "null"_json);
    }

    SECTION("evaluates comparator expression with greater operator")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::Greater,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::Greater,
            ast::ExpressionNode{
                ast::LiteralNode{"2"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();

        REQUIRE_FALSE(result1);
        REQUIRE(result2);
    }

    SECTION("evaluates comparator expression with greater operator "
            "to null with non number")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::Greater,
            ast::ExpressionNode{
                ast::LiteralNode{"false"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"null"}},
            ast::ComparatorExpressionNode::Comparator::Greater,
            ast::ExpressionNode{
                ast::LiteralNode{"2"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();

        REQUIRE(result1 == "null"_json);
        REQUIRE(result2 == "null"_json);
    }

    SECTION("evaluates comparator expression with not equal operator")
    {
        ast::ComparatorExpressionNode node1{
            ast::ExpressionNode{
                ast::LiteralNode{"5"}},
            ast::ComparatorExpressionNode::Comparator::NotEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        ast::ComparatorExpressionNode node2{
            ast::ExpressionNode{
                ast::LiteralNode{"2"}},
            ast::ComparatorExpressionNode::Comparator::NotEqual,
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}};
        Json trueResult = true;

        interpreter.visit(&node1);
        Json result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        Json result2 = interpreter.currentContext();

        REQUIRE_FALSE(result1);
        REQUIRE(result2);
    }

    SECTION("throws exception on evaluating comparator expression with "
            "unknown operator")
    {
        ast::ComparatorExpressionNode node;

        REQUIRE_THROWS_AS(interpreter.visit(&node), InvalidAgrument);
    }

    SECTION("evaluates comparator expression with rvalue referring to the same "
            "value on both sides")
    {
        ast::ComparatorExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"key"}},
            ast::ComparatorExpressionNode::Comparator::Equal,
            ast::ExpressionNode{
                ast::IdentifierNode{"key"}}};
        auto context = "{\"key\": \"value\"}"_json;
        interpreter.setContext(std::move(context));

        interpreter.visit(&node);
        Json result = interpreter.currentContext();

        REQUIRE(result == true);
    }

    SECTION("evaluates or expression")
    {
        ast::OrExpressionNode node{
            ast::ExpressionNode{},
            ast::ExpressionNode{}};
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)
               + OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.rightExpression)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates or expression to left expression's result if it's true "
            "with lvalue ref")
    {
        ast::OrExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        auto context = "{\"id1\": \"value1\", \"id2\": \"value2\"}"_json;
        interpreter.setContext(context);
        Json expectedResult = "value1";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 1);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates or expression to left expression's result if it's true "
            "with rvalue")
    {
        ast::OrExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        interpreter.setContext(
                    "{\"id1\": \"value1\", \"id2\": \"value2\"}"_json);
        Json expectedResult = "value1";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates or expression to right expression's result if left "
            "expression's result is false with lvalue ref")
    {
        ast::OrExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        auto context = "{\"id1\": \"\", \"id2\": \"value2\"}"_json;
        interpreter.setContext(context);
        Json expectedResult = "value2";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 1);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates or expression to right expression's result if left "
            "expression's result is false with rvalue")
    {
        ast::OrExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        interpreter.setContext("{\"id1\": \"\", \"id2\": \"value2\"}"_json);
        Json expectedResult = "value2";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates and expression")
    {
        ast::AndExpressionNode node{
            ast::ExpressionNode{},
            ast::ExpressionNode{}};
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();
        interpreterMock.get().setContext("true"_json);

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)
               + OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.rightExpression)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates and expression to right expression's result if left "
            "expression's result is true with lvalue ref")
    {
        ast::AndExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        auto context = "{\"id1\": \"value1\", \"id2\": \"value2\"}"_json;
        interpreter.setContext(context);
        Json expectedResult = "value2";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 1);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates and expression to right expression's result if left "
            "expression's result is true with rvalue")
    {
        ast::AndExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        interpreter.setContext(
                    "{\"id1\": \"value1\", \"id2\": \"value2\"}"_json);
        Json expectedResult = "value2";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates and expression to left expression's result if left "
            "expression's result is false with lvalue ref")
    {
        ast::AndExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        auto context = "{\"id1\": [], \"id2\": \"value2\"}"_json;
        interpreter.setContext(context);
        Json expectedResult = "[]"_json;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 1);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates and expression to left expression's result if left "
            "expression's result is false with rvalue")
    {
        ast::AndExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        interpreter.setContext("{\"id1\": [], \"id2\": \"value2\"}"_json);
        Json expectedResult = "[]"_json;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates child expression of paren expression")
    {
        ast::ParenExpressionNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
               .Using(&node.expression)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates pipe expression")
    {
        ast::PipeExpressionNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.leftExpression)
               + OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.rightExpression)).Once();
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates pipe expression by passing the left expression's result "
            "to the right expression")
    {
        ast::PipeExpressionNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"id1"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id2"}}};
        interpreter.setContext("{\"id1\": {\"id2\": \"value\"}}"_json);
        Json expectedResult = "value";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates current node expression")
    {
        ast::CurrentNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*))).AlwaysReturn();
        interpreter.setContext("\"value\""_json);

        interpreterMock.get().visit(&node);

        REQUIRE(interpreterMock.get().currentContext() == "value");
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates current node inside subexpression")
    {
        ast::SubexpressionNode node{
            ast::ExpressionNode{
                ast::CurrentNode{}},
            ast::ExpressionNode{
                ast::IdentifierNode{"id"}}};
        interpreter.setContext("{\"id\": \"value\"}"_json);
        Json expectedResult = "value";

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates child expression of filter expression with lvalue ref")
    {
        ast::FilterExpressionNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        auto context = "[1, 2, 3]"_json;
        interpreterMock.get().setContext(context);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.expression)).Exactly(3);
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates child expression of filter expression with rvalue")
    {
        ast::FilterExpressionNode node;
        Mock<Interpreter> interpreterMock(interpreter);
        interpreterMock.get().setContext("[1, 2, 3]"_json);
        When(OverloadedMethod(interpreterMock, visit,
                              void(const ast::ExpressionNode*)))
                .AlwaysReturn();

        interpreterMock.get().visit(&node);

        Verify(OverloadedMethod(interpreterMock, visit,
                                void(const ast::ExpressionNode*))
                    .Using(&node.expression)).Exactly(3);
        VerifyNoOtherInvocations(interpreterMock);
    }

    SECTION("evaluates filter expression on non array to null")
    {
        Json context = "{}"_json;
        interpreter.setContext(context);
        ast::FilterExpressionNode node;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == Json{});
    }

    SECTION("evaluates filter expression on arrays to an array filtered with "
            "the filter's subexpression with lvalue ref")
    {
        Json context = "[{\"id\": 1}, {\"id\": 2}, {\"id\": 3}]"_json;
        interpreter.setContext(context);
        ast::FilterExpressionNode node{
            ast::ExpressionNode{
                ast::ComparatorExpressionNode{
                    ast::ExpressionNode{
                        ast::IdentifierNode{"id"}},
                    ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
                    ast::ExpressionNode{
                        ast::LiteralNode{"2"}}}}};
        Json expectedResult = "[{\"id\": 2}, {\"id\": 3}]"_json;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);
    }

    SECTION("evaluates filter expression on arrays to an array filtered with "
            "the filter's subexpression with rvalue")
    {
        interpreter.setContext("[{\"id\": 1}, {\"id\": 2}, {\"id\": 3}]"_json);
        ast::FilterExpressionNode node{
            ast::ExpressionNode{
                ast::ComparatorExpressionNode{
                    ast::ExpressionNode{
                        ast::IdentifierNode{"id"}},
                    ast::ComparatorExpressionNode::Comparator::GreaterOrEqual,
                    ast::ExpressionNode{
                        ast::LiteralNode{"2"}}}}};
        Json expectedResult = "[{\"id\": 2}, {\"id\": 3}]"_json;

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == expectedResult);

    }

    SECTION("evaluates filter expression on arrays to an array filtered with "
            "the filter's subexpression 2 with lvalue ref")
    {
        Json context2 = "[{\"name\": \"a\"}, {\"name\": \"b\"}]"_json;
        interpreter.setContext(context2);
        ast::FilterExpressionNode node2{
            ast::ExpressionNode{
                ast::ComparatorExpressionNode{
                    ast::ExpressionNode{
                        ast::IdentifierNode{"name"}},
                    ast::ComparatorExpressionNode::Comparator::Equal,
                    ast::ExpressionNode{
                        ast::RawStringNode{"a"}}}}};
        Json expectedResult2 = "[{\"name\": \"a\"}]"_json;

        interpreter.visit(&node2);

        REQUIRE(interpreter.currentContext() == expectedResult2);
    }

    SECTION("evaluates filter expression on arrays to an array filtered with "
            "the filter's subexpression 2 with rvalue")
    {
        interpreter.setContext("[{\"name\": \"a\"}, {\"name\": \"b\"}]"_json);
        ast::FilterExpressionNode node2{
            ast::ExpressionNode{
                ast::ComparatorExpressionNode{
                    ast::ExpressionNode{
                        ast::IdentifierNode{"name"}},
                    ast::ComparatorExpressionNode::Comparator::Equal,
                    ast::ExpressionNode{
                        ast::RawStringNode{"a"}}}}};
        Json expectedResult2 = "[{\"name\": \"a\"}]"_json;

        interpreter.visit(&node2);

        REQUIRE(interpreter.currentContext() == expectedResult2);
    }

    SECTION("function expression evaluation throws on non existing function "
            "call")
    {
        ast::FunctionExpressionNode node{"foo"};

        REQUIRE_THROWS_AS(interpreter.visit(&node), UnknownFunction);
    }

    SECTION("abs function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "abs",
            {ast::ExpressionNode{
                ast::LiteralNode{"-3"}},
            ast::ExpressionNode{
                ast::LiteralNode{"5"}}}};
        ast::FunctionExpressionNode node2{"abs"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("abs function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "abs",
            {ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::LiteralNode{"true"}}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("abs function throws on inavlid json argument type")
    {
        ast::FunctionExpressionNode node{
            "abs",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates abs function on integer")
    {
        ast::FunctionExpressionNode node1{
            "abs",
            {ast::ExpressionNode{
                ast::LiteralNode{"-3"}}}};
        ast::FunctionExpressionNode node2{
            "abs",
            {ast::ExpressionNode{
                ast::LiteralNode{"5"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "3"_json);
        REQUIRE(result2 == "5"_json);
    }

    SECTION("evaluates abs function on float")
    {
        ast::FunctionExpressionNode node1{
            "abs",
            {ast::ExpressionNode{
                ast::LiteralNode{"-3.7"}}}};
        ast::FunctionExpressionNode node2{
            "abs",
            {ast::ExpressionNode{
                ast::LiteralNode{"5.8"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "3.7"_json);
        REQUIRE(result2 == "5.8"_json);
    }

    SECTION("avg function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "avg",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"avg"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("avg function throws on non array argument type")
    {
        ast::FunctionExpressionNode node{
            "avg",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("avg function throws on array argument non number item type")
    {
        ast::FunctionExpressionNode node{
            "avg",
            {ast::ExpressionNode{
                ast::LiteralNode{"[true, false]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates avg function")
    {
        ast::FunctionExpressionNode node{
            "avg",
            {ast::ExpressionNode{
                ast::LiteralNode{"[2, 7.5, 4.3, -17.8]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "-1"_json);
    }

    SECTION("evaluates avg function to null on empty list")
    {
        ast::FunctionExpressionNode node{
            "avg",
            {ast::ExpressionNode{
                ast::LiteralNode{"[]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "null"_json);
    }

    SECTION("contains function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"contains"};
        ast::FunctionExpressionNode node1{
            "contains",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "contains",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("contains function throws on non array or non string argument type")
    {
        ast::FunctionExpressionNode node{
            "contains",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates contains function on arrays")
    {
        ast::FunctionExpressionNode node1{
            "contains",
            {ast::ExpressionNode{
                ast::LiteralNode{"[2, 7.5, 4.3, -17.8]"}},
            ast::ExpressionNode{
                ast::LiteralNode{"2"}}}};
        ast::FunctionExpressionNode node2{
            "contains",
            {ast::ExpressionNode{
                ast::LiteralNode{"[2, 7.5, 4.3, -17.8]"}},
            ast::ExpressionNode{
                ast::LiteralNode{"3"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "true"_json);
        REQUIRE(result2 == "false"_json);
    }

    SECTION("evaluates contains function on strings")
    {
        ast::FunctionExpressionNode node1{
            "contains",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"The quick brown fox...\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"fox\""}}}};
        ast::FunctionExpressionNode node2{
            "contains",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"The quick brown fox...\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"dog\""}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "true"_json);
        REQUIRE(result2 == "false"_json);
    }

    SECTION("ceil function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "ceil",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"ceil"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("ceil function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "ceil",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates ceil function on integer")
    {
        ast::FunctionExpressionNode node1{
            "ceil",
            {ast::ExpressionNode{
                ast::LiteralNode{"-3"}}}};
        ast::FunctionExpressionNode node2{
            "ceil",
            {ast::ExpressionNode{
                ast::LiteralNode{"5"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "-3"_json);
        REQUIRE(result2 == "5"_json);
    }

    SECTION("evaluates ceil function on float")
    {
        ast::FunctionExpressionNode node1{
            "ceil",
            {ast::ExpressionNode{
                ast::LiteralNode{"-3.7"}}}};
        ast::FunctionExpressionNode node2{
            "ceil",
            {ast::ExpressionNode{
                ast::LiteralNode{"5.8"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "-3"_json);
        REQUIRE(result2 == "6"_json);
    }

    SECTION("ends_with function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"ends_with"};
        ast::FunctionExpressionNode node1{
            "ends_with",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "ends_with",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("ends_with function throws on non string argument types")
    {
        ast::FunctionExpressionNode node1{
            "ends_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node2{
            "ends_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node3{
            "ends_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates ends_with function")
    {
        ast::FunctionExpressionNode node1{
            "ends_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"The quick brown fox\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"fox\""}}}};
        ast::FunctionExpressionNode node2{
            "ends_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"The quick brown fox\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"dog\""}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "true"_json);
        REQUIRE(result2 == "false"_json);
    }

    SECTION("floor function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "floor",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"floor"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("floor function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "floor",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates floor function on integer")
    {
        ast::FunctionExpressionNode node1{
            "floor",
            {ast::ExpressionNode{
                ast::LiteralNode{"-3"}}}};
        ast::FunctionExpressionNode node2{
            "floor",
            {ast::ExpressionNode{
                ast::LiteralNode{"5"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "-3"_json);
        REQUIRE(result2 == "5"_json);
    }

    SECTION("evaluates floor function on float")
    {
        ast::FunctionExpressionNode node1{
            "floor",
            {ast::ExpressionNode{
                ast::LiteralNode{"-3.7"}}}};
        ast::FunctionExpressionNode node2{
            "floor",
            {ast::ExpressionNode{
                ast::LiteralNode{"5.8"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "-4"_json);
        REQUIRE(result2 == "5"_json);
    }

    SECTION("join function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"join"};
        ast::FunctionExpressionNode node1{
            "join",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "join",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("join function throws on non string and string array types")
    {
        ast::FunctionExpressionNode node1{
            "join",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node2{
            "join",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node3{
            "join",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{
                ast::LiteralNode{"[\"string\"]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates join function")
    {
        ast::FunctionExpressionNode node{
            "join",
            {ast::ExpressionNode{
                ast::LiteralNode{"\";\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"[\"string1\", \"string2\", \"string3\"]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()
                == "\"string1;string2;string3\""_json);
    }

    SECTION("keys function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "keys",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"keys"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("keys function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "keys",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates keys function")
    {
        ast::FunctionExpressionNode node{
            "keys",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id1\": 1, \"id2\": 2}"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()
                == "[\"id1\", \"id2\"]"_json);
    }

    SECTION("length function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "length",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"length"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("length function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "length",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates length function")
    {
        ast::FunctionExpressionNode node1{
            "length",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id1\": 1, \"id2\": 2}"}}}};
        ast::FunctionExpressionNode node2{
            "length",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}}}};
        ast::FunctionExpressionNode node3{
            "length",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        auto result3 = interpreter.currentContext();

        REQUIRE(result1 == "2"_json);
        REQUIRE(result2 == "3"_json);
        REQUIRE(result3 == "6"_json);
    }

    SECTION("map function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"map"};
        ast::FunctionExpressionNode node1{
            "map",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "map",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("map function throws on invalid argument types")
    {
        ast::FunctionExpressionNode node1{
            "map",
            {ast::ExpressionNode{},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node2{
            "map",
            {ast::ExpressionArgumentNode{
                ast::ExpressionNode{}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node3{
            "map",
            {ast::ExpressionNode{},
            ast::ExpressionNode{
                ast::LiteralNode{"[]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates map function with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "map",
            {ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}},
            ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context
                = "{\"foo\": [{\"id\": 1}, {\"id\": 2}, {\"id2\": 3}]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2, null]"_json);
    }

    SECTION("evaluates map function with rvalue")
    {
        ast::FunctionExpressionNode node{
            "map",
            {ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}},
            ast::ExpressionNode{
                ast::LiteralNode{"[{\"id\": 1}, {\"id\": 2}, {\"id2\": 3}]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2, null]"_json);
    }

    SECTION("max function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "max",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"max"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("max function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("max function throws on array argument invalid item type")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::LiteralNode{"[true, false]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("max function throws on array argument heterogeneous item types")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, \"string\"]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates max function on number array with rvalue")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::LiteralNode{"[2, 3, 1]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "3"_json);
    }

    SECTION("evaluates max function on number array with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [2, 3, 1]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "3"_json);
    }

    SECTION("evaluates max function on string array with rvalue")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::LiteralNode{"[\"bar\", \"foo\", \"baz\"]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"foo\""_json);
    }

    SECTION("evaluates max function on string array with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [\"bar\", \"foo\", \"baz\"]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"foo\""_json);
    }

    SECTION("max function returns null on empty array")
    {
        ast::FunctionExpressionNode node{
            "max",
            {ast::ExpressionNode{
                ast::LiteralNode{"[]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "null"_json);
    }

    SECTION("max_by function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"max_by"};
        ast::FunctionExpressionNode node1{
            "max_by",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "max_by",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("max_by function throws on invalid argument types")
    {
        ast::FunctionExpressionNode node1{
            "max_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{
            "max_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "max_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentType);
    }

    SECTION("max_by function throws on invalid expression result type")
    {
        ast::FunctionExpressionNode node{
            "max_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates max_by function on empty array to null")
    {
        ast::FunctionExpressionNode node{
            "max_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "null"_json);
    }

    SECTION("evaluates max_by function with rvalue")
    {
        ast::FunctionExpressionNode node{
            "max_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[{\"id\": 3}, {\"id\": 5}, {\"id\": 1}]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "{\"id\": 5}"_json);
    }

    SECTION("evaluates max_by function with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "max_by",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};
        auto context = "{\"foo\": [{\"id\": 3}, {\"id\": 5}, "
                       "{\"id\": 1}]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "{\"id\": 5}"_json);
    }

    SECTION("merge function throws on non object argument types")
    {
        ast::FunctionExpressionNode node{
            "merge",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id\": 2}"}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates merge function on objects with rvalue")
    {
        ast::FunctionExpressionNode node{
            "merge",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id1\": 0, \"id2\": 2}"}},
            ast::ExpressionNode{
                ast::LiteralNode{"{\"id1\": 1, \"id3\": 3}"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()
                == "{\"id1\": 1, \"id2\":2, \"id3\": 3}"_json);
    }

    SECTION("evaluates merge function on objects with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "merge",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"bar"}}}};
        auto context = "{\"foo\": {\"id1\": 0, \"id2\": 2},"
                       "\"bar\": {\"id1\": 1, \"id3\": 3}}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()
                == "{\"id1\": 1, \"id2\":2, \"id3\": 3}"_json);
    }

    SECTION("min function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "min",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"min"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("min function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "min",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("min function throws on array argument invalid item type")
    {
        ast::FunctionExpressionNode node{
            "min",
            {ast::ExpressionNode{
                ast::LiteralNode{"[true, false]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("min function throws on array argument heterogeneous item types")
    {
        ast::FunctionExpressionNode node{
            "min",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, \"string\"]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates min function on number array with rvalue")
    {
        ast::FunctionExpressionNode node{
            "min",
            {ast::ExpressionNode{
                ast::LiteralNode{"[2, 3, 1]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "1"_json);
    }

    SECTION("evaluates min function on number array with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "min",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [2, 3, 1]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "1"_json);
    }

    SECTION("evaluates min function on string array with rvalue")
    {
        ast::FunctionExpressionNode node{
            "min",
            {ast::ExpressionNode{
                ast::LiteralNode{"[\"bar\", \"foo\", \"baz\"]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"bar\""_json);
    }

    SECTION("evaluates min function on string array with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "min",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [\"bar\", \"foo\", \"baz\"]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"bar\""_json);
    }

    SECTION("min_by function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"min_by"};
        ast::FunctionExpressionNode node1{
            "min_by",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "min_by",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("min_by function throws on invalid argument types")
    {
        ast::FunctionExpressionNode node1{
            "min_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{
            "min_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "min_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentType);
    }

    SECTION("min_by function throws on invalid expression result type")
    {
        ast::FunctionExpressionNode node{
            "min_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates min_by function on empty array to null")
    {
        ast::FunctionExpressionNode node{
            "min_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "null"_json);
    }

    SECTION("evaluates min_by function with rvalue")
    {
        ast::FunctionExpressionNode node{
            "min_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[{\"id\": 3}, {\"id\": 5}, {\"id\": 1}]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "{\"id\": 1}"_json);
    }

    SECTION("evaluates min_by function with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "min_by",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};
        auto context = "{\"foo\": [{\"id\": 3}, {\"id\": 5}, "
                       "{\"id\": 1}]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "{\"id\": 1}"_json);
    }

    SECTION("not_null function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"not_null"};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
    }

    SECTION("not_null function throws on non JSON argument type")
    {
        ast::FunctionExpressionNode node{
            "not_null",
            {ast::ExpressionArgumentNode{
                ast::ExpressionNode{}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates not_null function with rvalue")
    {
        ast::FunctionExpressionNode node{
            "not_null",
            {ast::ExpressionNode{
                ast::LiteralNode{"null"}},
            ast::ExpressionNode{
                ast::LiteralNode{"[]"}},
            ast::ExpressionNode{
                ast::LiteralNode{"null"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[]"_json);
    }

    SECTION("evaluates not_null function with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "not_null",
            {ast::ExpressionNode{
                ast::IdentifierNode{"a"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"b"}},
            ast::ExpressionNode{
                ast::IdentifierNode{"c"}}}};
        auto context = "{\"a\": null, \"b\": [], \"c\": null}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[]"_json);
    }

    SECTION("not_null returns null if no not null values are found")
    {
        ast::FunctionExpressionNode node{
            "not_null",
            {ast::ExpressionNode{
                ast::LiteralNode{"null"}},
            ast::ExpressionNode{
                ast::LiteralNode{"null"}},
            ast::ExpressionNode{
                ast::LiteralNode{"null"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "null"_json);
    }

    SECTION("reverse function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"reverse"};
        ast::FunctionExpressionNode node2{
            "reverse",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("reverse function throws on non array or non string argument type")
    {
        ast::FunctionExpressionNode node{
            "reverse",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates reverse function on array with rvalue")
    {
        ast::FunctionExpressionNode node{
            "reverse",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[3, 2, 1]"_json);
    }

    SECTION("evaluates reverse function on array with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "reverse",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [1, 2, 3]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[3, 2, 1]"_json);
    }

    SECTION("evaluates reverse function on strings with rvalue")
    {
        ast::FunctionExpressionNode node{
            "reverse",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"abc\""}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"cba\""_json);
    }

    SECTION("evaluates reverse function on strings with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "reverse",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\":\"abc\"}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"cba\""_json);
    }

    SECTION("sort function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"sort"};
        ast::FunctionExpressionNode node2{
            "sort",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("sort function throws on non array type")
    {
        ast::FunctionExpressionNode node{
            "sort",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("sort function throws on non number or string item type")
    {
        ast::FunctionExpressionNode node{
            "sort",
            {ast::ExpressionNode{
                ast::LiteralNode{"[true, false]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("sort function throws on array argument heterogeneous item types")
    {
        ast::FunctionExpressionNode node{
            "sort",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, \"string\"]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates sort function on array of numbers with rvalue")
    {
        ast::FunctionExpressionNode node{
            "sort",
            {ast::ExpressionNode{
                ast::LiteralNode{"[3, 1, 2]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2, 3]"_json);
    }

    SECTION("evaluates sort function on array of numbers with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "sort",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [3, 1, 2]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2, 3]"_json);
    }

    SECTION("evaluates sort function on array of strings with rvalue")
    {
        ast::FunctionExpressionNode node{
            "sort",
            {ast::ExpressionNode{
                ast::LiteralNode{"[\"b\", \"c\", \"a\"]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[\"a\", \"b\", \"c\"]"_json);
    }

    SECTION("evaluates sort function on array of strings with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "sort",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [\"b\", \"c\", \"a\"]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[\"a\", \"b\", \"c\"]"_json);
    }

    SECTION("sort_by function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"sort_by"};
        ast::FunctionExpressionNode node1{
            "sort_by",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "sort_by",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("sort_by function throws on invalid argument types")
    {
        ast::FunctionExpressionNode node1{
            "sort_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{
            "sort_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "sort_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentType);
    }

    SECTION("sort_by function throws on invalid expression result type")
    {
        ast::FunctionExpressionNode node{
            "sort_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("sort_by function throws on inconsistent expression result type")
    {
        ast::FunctionExpressionNode node{
            "sort_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[{\"id\": 3}, {\"id\": 5}, {\"id\":\"s\"}]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates sort_by function with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "sort_by",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};
        auto context = "{\"foo\": [{\"id\": 3}, {\"id\": 5}, {\"id\": 1},"
                       "{\"id\": 4}, {\"id\": 2}]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()
                == "[{\"id\": 1}, {\"id\": 2}, {\"id\": 3}, {\"id\": 4}, "
                   "{\"id\": 5}]"_json);
    }

    SECTION("evaluates sort_by function with rvalue")
    {
        ast::FunctionExpressionNode node{
            "sort_by",
            {ast::ExpressionNode{
                ast::LiteralNode{"[{\"id\": 3}, {\"id\": 5}, {\"id\": 1},"
                                 "{\"id\": 4}, {\"id\": 2}]"}},
            ast::ExpressionArgumentNode{
                ast::ExpressionNode{
                    ast::IdentifierNode{"id"}}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext()
                == "[{\"id\": 1}, {\"id\": 2}, {\"id\": 3}, {\"id\": 4}, "
                   "{\"id\": 5}]"_json);
    }

    SECTION("starts_with function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node0{"starts_with"};
        ast::FunctionExpressionNode node1{
            "starts_with",
            {ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node3{
            "starts_with",
            {ast::ExpressionNode{},
            ast::ExpressionNode{},
            ast::ExpressionNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node0),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentArity);
    }

    SECTION("starts_with function throws on non string argument types")
    {
        ast::FunctionExpressionNode node1{
            "starts_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node2{
            "starts_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node3{
            "starts_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentType);
        REQUIRE_THROWS_AS(interpreter.visit(&node3),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates starts_with function")
    {
        ast::FunctionExpressionNode node1{
            "starts_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"The quick brown fox\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"The\""}}}};
        ast::FunctionExpressionNode node2{
            "starts_with",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"The quick brown fox\""}},
            ast::ExpressionNode{
                ast::LiteralNode{"\"fox\""}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();

        REQUIRE(result1 == "true"_json);
        REQUIRE(result2 == "false"_json);
    }

    SECTION("sum function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "sum",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"sum"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("sum function throws on non array argument type")
    {
        ast::FunctionExpressionNode node{
            "sum",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("sum function throws on array argument non number item type")
    {
        ast::FunctionExpressionNode node{
            "sum",
            {ast::ExpressionNode{
                ast::LiteralNode{"[true, false]"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates sum function")
    {
        ast::FunctionExpressionNode node{
            "sum",
            {ast::ExpressionNode{
                ast::LiteralNode{"[2, 7.5, 4.3, -17.8]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "-4"_json);
    }

    SECTION("to_array function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "to_array",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"to_array"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("to_array function throws on non JSON argument type")
    {
        ast::FunctionExpressionNode node{
            "to_array",
            {ast::ExpressionArgumentNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates to_array function on array to the array itself "
            "with rvalue")
    {
        ast::FunctionExpressionNode node{
            "to_array",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContextValue().which() == 0);
        REQUIRE(interpreter.currentContext() == "[1, 2, 3]"_json);
    }

    SECTION("evaluates to_array function on array to the array itself "
            "with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "to_array",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": [1, 2, 3]}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2, 3]"_json);
    }

    SECTION("evaluates to_array function on non array to a single element "
            "array")
    {
        ast::FunctionExpressionNode node1{
            "to_array",
            {ast::ExpressionNode{
                ast::LiteralNode{"1"}}}};
        ast::FunctionExpressionNode node2{
            "to_array",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node3{
            "to_array",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}}}};
        ast::FunctionExpressionNode node4{
            "to_array",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id\": 1}"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        auto result3 = interpreter.currentContext();
        interpreter.visit(&node4);
        auto result4 = interpreter.currentContext();

        REQUIRE(result1 == "[1]"_json);
        REQUIRE(result2 == "[true]"_json);
        REQUIRE(result3 == "[\"string\"]"_json);
        REQUIRE(result4 == "[{\"id\": 1}]"_json);
    }

    SECTION("to_string function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "to_string",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"to_string"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("to_string function throws on non JSON argument type")
    {
        ast::FunctionExpressionNode node{
            "to_string",
            {ast::ExpressionArgumentNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates to_string function on string to the string itself "
            "with rvalue")
    {
        ast::FunctionExpressionNode node{
            "to_string",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"string\""_json);
    }

    SECTION("evaluates to_string function on string to the string itself "
            "with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "to_string",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": \"string\"}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"string\""_json);
    }

    SECTION("evaluates to_string function on non string to the string "
            "representation of argument")
    {
        ast::FunctionExpressionNode node1{
            "to_string",
            {ast::ExpressionNode{
                ast::LiteralNode{"1"}}}};
        ast::FunctionExpressionNode node2{
            "to_string",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node3{
            "to_string",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}}}};
        ast::FunctionExpressionNode node4{
            "to_string",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id1\": 1, \"id2\": 2}"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        auto result3 = interpreter.currentContext();
        interpreter.visit(&node4);
        auto result4 = interpreter.currentContext();

        REQUIRE(result1 == "\"1\""_json);
        REQUIRE(result2 == "\"true\""_json);
        REQUIRE(result3 == "\"[1,2,3]\""_json);
        REQUIRE(result4 == "\"{\\\"id1\\\":1,\\\"id2\\\":2}\""_json);
    }

    SECTION("to_number function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "to_number",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"to_number"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("to_number function throws on non JSON argument type")
    {
        ast::FunctionExpressionNode node{
            "to_number",
            {ast::ExpressionArgumentNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates to_number function on number and string to number "
            "with rvalue")
    {
        ast::FunctionExpressionNode node1{
            "to_number",
            {ast::ExpressionNode{
                ast::LiteralNode{"1.2"}}}};
        ast::FunctionExpressionNode node2{
            "to_number",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"-2.7\""}}}};
        ast::FunctionExpressionNode node3{
            "to_number",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"not a number\""}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        auto result3 = interpreter.currentContext();

        REQUIRE(result1 == "1.2"_json);
        REQUIRE(result2 == "-2.7"_json);
        REQUIRE(result3 == "null"_json);
    }

    SECTION("evaluates to_number function on number and string to number "
            "with lvalue ref")
    {
        ast::FunctionExpressionNode node1{
            "to_number",
            {ast::ExpressionNode{
                ast::IdentifierNode{"a"}}}};
        ast::FunctionExpressionNode node2{
            "to_number",
            {ast::ExpressionNode{
                ast::IdentifierNode{"b"}}}};
        ast::FunctionExpressionNode node3{
            "to_number",
            {ast::ExpressionNode{
                ast::IdentifierNode{"c"}}}};
        Json context {
            {"a", 1.2},
            {"b", "-2.7"},
            {"c", "not a number"}
        };

        interpreter.setContext(context);
        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.setContext(context);
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();
        interpreter.setContext(context);
        interpreter.visit(&node3);
        auto result3 = interpreter.currentContext();

        REQUIRE(result1 == "1.2"_json);
        REQUIRE(result2 == "-2.7"_json);
        REQUIRE(result3 == "null"_json);
    }

    SECTION("evaluates to_number function on non numbers and strings to null")
    {
        ast::FunctionExpressionNode node1{
            "to_number",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node2{
            "to_number",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}}}};
        ast::FunctionExpressionNode node3{
            "to_number",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id1\": 1, \"id2\": 2}"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        auto result3 = interpreter.currentContext();

        REQUIRE(result1 == "null"_json);
        REQUIRE(result2 == "null"_json);
        REQUIRE(result3 == "null"_json);
    }

    SECTION("type function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "type",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"type"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("type function throws on non JSON argument type")
    {
        ast::FunctionExpressionNode node{
            "type",
            {ast::ExpressionArgumentNode{}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates type function to the name of the JSON value type")
    {
        ast::FunctionExpressionNode node1{
            "type",
            {ast::ExpressionNode{
                ast::LiteralNode{"1.2"}}}};
        ast::FunctionExpressionNode node2{
            "type",
            {ast::ExpressionNode{
                ast::LiteralNode{"\"string\""}}}};
        ast::FunctionExpressionNode node3{
            "type",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};
        ast::FunctionExpressionNode node4{
            "type",
            {ast::ExpressionNode{
                ast::LiteralNode{"[1, 2, 3]"}}}};
        ast::FunctionExpressionNode node5{
            "type",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id\": 1}"}}}};
        ast::FunctionExpressionNode node6{
            "type",
            {ast::ExpressionNode{
                ast::LiteralNode{"null"}}}};

        interpreter.visit(&node1);
        auto result1 = interpreter.currentContext();
        interpreter.visit(&node2);
        auto result2 = interpreter.currentContext();
        interpreter.visit(&node3);
        auto result3 = interpreter.currentContext();
        interpreter.visit(&node4);
        auto result4 = interpreter.currentContext();
        interpreter.visit(&node5);
        auto result5 = interpreter.currentContext();
        interpreter.visit(&node6);
        auto result6 = interpreter.currentContext();

        REQUIRE(result1 == "\"number\""_json);
        REQUIRE(result2 == "\"string\""_json);
        REQUIRE(result3 == "\"boolean\""_json);
        REQUIRE(result4 == "\"array\""_json);
        REQUIRE(result5 == "\"object\""_json);
        REQUIRE(result6 == "\"null\""_json);
    }

    SECTION("values function throws on invalid number of arguments")
    {
        ast::FunctionExpressionNode node1{
            "values",
            {ast::ExpressionNode{},
            ast::ExpressionNode{}}};
        ast::FunctionExpressionNode node2{"values"};

        REQUIRE_THROWS_AS(interpreter.visit(&node1),
                          InvalidFunctionArgumentArity);
        REQUIRE_THROWS_AS(interpreter.visit(&node2),
                          InvalidFunctionArgumentArity);
    }

    SECTION("values function throws on inavlid argument type")
    {
        ast::FunctionExpressionNode node{
            "values",
            {ast::ExpressionNode{
                ast::LiteralNode{"true"}}}};

        REQUIRE_THROWS_AS(interpreter.visit(&node),
                          InvalidFunctionArgumentType);
    }

    SECTION("evaluates values function with rvalue")
    {
        ast::FunctionExpressionNode node{
            "values",
            {ast::ExpressionNode{
                ast::LiteralNode{"{\"id1\": 1, \"id2\": 2}"}}}};

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2]"_json);
    }

    SECTION("evaluates values function with lvalue ref")
    {
        ast::FunctionExpressionNode node{
            "values",
            {ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}}};
        auto context = "{\"foo\": {\"id1\": 1, \"id2\": 2}}"_json;
        interpreter.setContext(context);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "[1, 2]"_json);
    }

    SECTION("evaluating an expression argument node directly does nothing")
    {
        ast::ExpressionArgumentNode node{
            ast::ExpressionNode{
                ast::IdentifierNode{"foo"}}};
        interpreter.setContext("\"test\""_json);

        interpreter.visit(&node);

        REQUIRE(interpreter.currentContext() == "\"test\""_json);
    }
}
