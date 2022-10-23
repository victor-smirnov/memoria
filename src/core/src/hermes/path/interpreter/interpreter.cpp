/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
** Copyright (c) 2022 Victor Smirnov
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
#include "interpreter.h"
#include "../ast/allnodes.h"
#include "memoria/core/hermes/path/exceptions.h"
#include "contextvaluevisitoradaptor.h"
#include <numeric>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/hana.hpp>
#include <boost/type_index.hpp>

namespace memoria::hermes::path { namespace interpreter {

namespace rng = boost::range;
namespace alg = boost::algorithm;

template <typename DT>
hermes::DataObjectPtr<DT> wrap_DO(DTTViewType<DT> view) {
    return hermes::DocView::wrap_dataobject<DT>(view);
}


hermes::GenericArrayPtr Interpreter::wrap_array(const std::vector<Json>& array) {
    auto doc = hermes::DocView::make_pooled();
    auto arr = doc->set_generic_array();

    for (const auto& item: array) {
        arr->append(item);
    }

    return arr;
}

hermes::GenericArrayPtr make_array() {
    auto doc = hermes::DocView::make_pooled();
    return doc->set_generic_array();
}

hermes::GenericMapPtr make_map() {
    auto doc = hermes::DocView::make_pooled();
    return doc->set_generic_map();
}

Interpreter::Interpreter()
    : AbstractVisitor{}
{
    // initialize JMESPath function name to function implementation mapping
    using std::placeholders::_1;
    using std::bind;
    using Descriptor = FunctionDescriptor;
    using FunctionType = void(Interpreter::*)(FunctionArgumentList&);
    using MaxFunctionType = void(Interpreter::*)(FunctionArgumentList&,
                                                  const JsonComparator&);
    auto exactlyOne = bind(std::equal_to<size_t>{}, _1, 1);
    auto exactlyTwo = bind(std::equal_to<size_t>{}, _1, 2);
    auto zeroOrMore = bind(std::greater_equal<size_t>{}, _1, 0);
    auto oneOrMore = bind(std::greater_equal<size_t>{}, _1, 1);
    auto mapPtr = static_cast<FunctionType>(&Interpreter::map);
    auto reversePtr = static_cast<FunctionType>(&Interpreter::reverse);
    auto sortPtr = static_cast<FunctionType>(&Interpreter::sort);
    auto sortByPtr = static_cast<FunctionType>(&Interpreter::sortBy);
    auto toArrayPtr = static_cast<FunctionType>(&Interpreter::toArray);
    auto toStringPtr = static_cast<FunctionType>(&Interpreter::toString);
    auto toDoublePtr = static_cast<FunctionType>(&Interpreter::toDouble);
    auto toBigIntPtr = static_cast<FunctionType>(&Interpreter::toBigInt);
    auto toBooleanPtr = static_cast<FunctionType>(&Interpreter::toBoolean);
    auto valuesPtr = static_cast<FunctionType>(&Interpreter::values);
    auto maxPtr = static_cast<MaxFunctionType>(&Interpreter::max);
    auto maxByPtr = static_cast<MaxFunctionType>(&Interpreter::maxBy);
    m_functionMap = {
        {"abs", Descriptor{exactlyOne, true,
                           bind(&Interpreter::abs, this, _1)}},
        {"avg",  Descriptor{exactlyOne, true,
                            bind(&Interpreter::avg, this, _1)}},
        {"contains", Descriptor{exactlyTwo, false,
                                bind(&Interpreter::contains, this, _1)}},
        {"ceil", Descriptor{exactlyOne, true,
                            bind(&Interpreter::ceil, this, _1)}},
        {"ends_with", Descriptor{exactlyTwo, false,
                                 bind(&Interpreter::endsWith, this, _1)}},
        {"floor", Descriptor{exactlyOne, true,
                             bind(&Interpreter::floor, this, _1)}},
        {"join", Descriptor{exactlyTwo, false,
                            bind(&Interpreter::join, this, _1)}},
        {"keys", Descriptor{exactlyOne, true,
                            bind(&Interpreter::keys, this, _1)}},
        {"length", Descriptor{exactlyOne, true,
                              bind(&Interpreter::length, this, _1)}},
        {"map", Descriptor{exactlyTwo, true,
                           bind(mapPtr, this, _1)}},
        {"max", Descriptor{exactlyOne, true,
                           bind(maxPtr, this, _1, hermes::Less{})}},
        {"max_by", Descriptor{exactlyTwo, true,
                              bind(maxByPtr, this, _1, hermes::Less{})}},
        {"merge", Descriptor{zeroOrMore, false,
                             bind(&Interpreter::merge, this, _1)}},
        {"min", Descriptor{exactlyOne, true,
                           bind(maxPtr, this, _1, hermes::Greater{})}},
        {"min_by", Descriptor{exactlyTwo, true,
                              bind(maxByPtr, this, _1, hermes::Greater{})}},
        {"not_null", Descriptor{oneOrMore, false,
                                bind(&Interpreter::notNull, this, _1)}},
        {"reverse", Descriptor{exactlyOne, true,
                               bind(reversePtr, this, _1)}},
        {"sort",  Descriptor{exactlyOne, true,
                             bind(sortPtr, this, _1)}},
        {"sort_by", Descriptor{exactlyTwo, true,
                               bind(sortByPtr, this, _1)}},
        {"starts_with", Descriptor{exactlyTwo, false,
                                   bind(&Interpreter::startsWith, this, _1)}},
        {"sum", Descriptor{exactlyOne, true,
                           bind(&Interpreter::sum, this, _1)}},
        {"to_array", Descriptor{exactlyOne, true,
                                bind(toArrayPtr, this, _1)}},
        {"to_string", Descriptor{exactlyOne, true,
                                 bind(toStringPtr, this, _1)}},
        {"to_double", Descriptor{exactlyOne, true,
                                 bind(toDoublePtr, this, _1)}},
        {"to_bigint", Descriptor{exactlyOne, true,
                                 bind(toBigIntPtr, this, _1)}},
        {"to_boolean", Descriptor{exactlyOne, true,
                                 bind(toBooleanPtr, this, _1)}},
        {"type", Descriptor{exactlyOne, true,
                            bind(&Interpreter::type, this, _1)}},
        {"values", Descriptor{exactlyOne, true,
                              bind(valuesPtr, this, _1)}}
    };
}

void Interpreter::evaluateProjection(const ast::ExpressionNode *expression)
{
    using std::placeholders::_1;
    // move the current context into a temporary variable in case it holds
    // a value, since the context member variable will get overwritten during
    // the evaluation of the projection
    evaluateProjection(expression, std::move(m_context));
}

template <typename JsonT>
void Interpreter::evaluateProjection(const ast::ExpressionNode* expression,
                                      JsonT&& context)
{
    using std::placeholders::_1;
    // evaluate the projection if the context holds an array
    if (context->is_generic_array())
    {
        auto ctx_array = context->as_generic_array();

        // create the array of results
        auto result = make_array();

        // iterate over the array
        for (auto item: *ctx_array)
        {
            // move the item into the context or create an lvalue reference
            // depending on the type of the context variable
            m_context = assignContextValue(std::move(item));
            // evaluate the expression
            visit(expression);
            // if the result of the expression is not null
            if (!getJsonValue(m_context)->is_null())
            {
                // add the result of the expression to the results array
                result->append(m_context);
            }
        }

        // set the results of the projection
        m_context = result->as_value();
    }
    // otherwise evaluate to null
    else {
        m_context = {};
    }
}

void Interpreter::visit(const ast::AbstractNode *node)
{
    node->accept(this);
}

void Interpreter::visit(const ast::ExpressionNode *node)
{
    node->accept(this);
}

void Interpreter::visit(const ast::IdentifierNode *node)
{
    visit(node, std::move(m_context));
}

template <typename JsonT>
void Interpreter::visit(const ast::IdentifierNode *node, JsonT &&context)
{
    // evaluete the identifier if the context holds an object
    if (context->is_generic_map())
    {
        auto map = context->as_generic_map();
        try
        {
            // assign either a const reference of the result or move the result
            // into the context depending on the type of the context parameter

            auto vv = map->get(node->identifier);
            m_context = assignContextValue(std::move(vv));
            return;
        }
        catch (const nlohmann::json::out_of_range&) {}
    }
    // otherwise evaluate to null
    m_context = {};
}

void Interpreter::visit(const ast::RawStringNode *node)
{
    auto doc = hermes::DocView::make_pooled();
    m_context = doc->set_dataobject<Varchar>(node->rawString)->as_value();
}

void Interpreter::visit(const ast::LiteralNode *node)
{
    auto doc = hermes::DocView::make_pooled();
    m_context = doc->set_dataobject<Varchar>(node->literal)->as_value();
}

void Interpreter::visit(const ast::SubexpressionNode *node)
{
    node->accept(this);
}

void Interpreter::visit(const ast::IndexExpressionNode *node)
{
    // evaluate the left side expression
    visit(&node->leftExpression);
    // evaluate the index expression if the context holds an array
    if (getJsonValue(m_context)->is_generic_array())
    {
        // evaluate the bracket specifier
        visit(&node->bracketSpecifier);
        // if the index expression also defines a projection then evaluate it
        if (node->isProjection())
        {
            evaluateProjection(&node->rightExpression);
        }
    }
    // otherwise evaluate to null
    else
    {
        m_context = {};
    }
}

void Interpreter::visit(const ast::ArrayItemNode *node)
{
    visit(node, std::move(m_context));
}

template <typename JsonT>
void Interpreter::visit(const ast::ArrayItemNode *node, JsonT &&context)
{
    // evaluate the array item expression if the context holds an array
    if (context->is_generic_array())
    {
        auto arr = context->as_generic_array();

        // normalize the index value
        auto arrayIndex = node->index;
        if (arrayIndex < 0)
        {
            arrayIndex += arr->size();
        }

        // evaluate the expression if the index is not out of range
        if ((arrayIndex >= 0) && (arrayIndex < arr->size()))
        {
            // assign either a const reference of the result or move the result
            // into the context depending on the type of the context parameter
            auto index = static_cast<size_t>(arrayIndex);
            m_context = assignContextValue(std::move(arr->get(index)));
            return;
        }
    }
    // otherwise evaluate to null
    m_context = {};
}

void Interpreter::visit(const ast::FlattenOperatorNode *node)
{
    visit(node, std::move(m_context));
}

template <typename JsonT>
void Interpreter::visit(const ast::FlattenOperatorNode*, JsonT&& context)
{
    // evaluate the flatten operation if the context holds an array
    if (context->is_generic_array())
    {
        auto ctx_array = context->as_generic_array();

        auto doc = hermes::DocView::make_pooled();
        auto result = doc->set_generic_array();

        // iterate over the array
        for (auto item: *ctx_array)
        {
            // if the item is an array append or move every one of its items
            // to the end of the results variable
            if (item->is_generic_array())
            {
                auto a0 = item->as_generic_array();
                for (auto item2: *a0) {
                    result->append(item2);
                }
            }
            // otherwise append or move the item
            else
            {
                result->append(std::move(item));
            }
        }
        // set the results of the flatten operation
        m_context = result->as_value();
    }
    // otherwise evaluate to null
    else
    {
        m_context = {};
    }
}

void Interpreter::visit(const ast::BracketSpecifierNode *node)
{
    node->accept(this);
}

void Interpreter::visit(const ast::SliceExpressionNode *node)
{
    visit(node, std::move(m_context));
}

template <typename JsonT>
void Interpreter::visit(const ast::SliceExpressionNode* node, JsonT&& context)
{
    // evaluate the slice operation if the context holds an array
    if (context->is_generic_array())
    {
        auto ctx_array = context->as_generic_array();

        Index startIndex = 0;
        Index stopIndex = 0;
        Index step = 1;
        size_t length = ctx_array->size();

        // verify the validity of slice indeces and normalize their values
        if (node->step)
        {
            if (*node->step == 0)
            {
                BOOST_THROW_EXCEPTION(InvalidValue{});
            }
            step = *node->step;
        }
        if (!node->start)
        {
            startIndex = step < 0 ? length - 1: 0;
        }
        else
        {
            startIndex = adjustSliceEndpoint(length, *node->start, step);
        }
        if (!node->stop)
        {
            stopIndex = step < 0 ? -1 : Index{length};
        }
        else
        {
            stopIndex = adjustSliceEndpoint(length, *node->stop, step);
        }

        // create the array of results
        //Json result(Json::value_t::array);

        auto doc = hermes::DocView::make_pooled();
        auto result = doc->set_generic_array();

        // iterate over the array
        for (auto i = startIndex;
             step > 0 ? (i < stopIndex) : (i > stopIndex);
             i += step)
        {
            // append a copy of the item at arrayIndex or move it into the
            // result array depending on the type of the context variable
            size_t arrayIndex = static_cast<uint64_t>(i);
            result->append(std::move(ctx_array->get(arrayIndex)));
        }

        // set the results of the projection
        m_context = result->as_value();
    }
    // otherwise evaluate to null
    else {
        m_context = {};
    }
}

void Interpreter::visit(const ast::ListWildcardNode*)
{
    // evaluate a list wildcard operation to null if the context isn't an array
    if (!getJsonValue(m_context)->is_generic_array()) {
        m_context = {};
    }
}

void Interpreter::visit(const ast::HashWildcardNode *node)
{
    visit(node, std::move(m_context));
}

template <typename JsonT>
void Interpreter::visit(const ast::HashWildcardNode* node, JsonT&& context)
{
    // evaluate the hash wildcard operation if the context holds an array
    if (context->is_generic_map())
    {
        auto ctx_map = context->as_generic_map();

        auto result = make_array();

        ctx_map->for_each([&](auto, auto value){
            result->append(value);
        });

        // set the results of the projection
        m_context = result->as_value();
    }
    // otherwise evaluate to null
    else
    {
        m_context = {};
    }
    // evaluate the projected sub expression
    evaluateProjection(&node->rightExpression);
}

void Interpreter::visit(const ast::MultiselectListNode *node)
{
    // evaluate the multiselect list opration if the context doesn't holds null
    if (!getJsonValue(m_context)->is_null())
    {
        auto doc = hermes::DocView::make_pooled();
        auto result = doc->set_generic_array();

        // move the current context into a temporary variable in case it holds
        // a value, since the context member variable will get overwritten
        // during  the evaluation of sub expressions
        ContextValue contextValue {std::move(m_context)};
        // iterate over the list of subexpressions
        for (auto& expression: node->expressions)
        {
            // assign a const lvalue ref to the context
            m_context = assignContextValue(getJsonValue(contextValue));
            // evaluate the subexpression
            visit(&expression);
            // copy the result of the subexpression into the list of results
            result->append(getJsonValue(m_context));
        }
        // set the results of the projection
        m_context = result->as_value();
    }
}

void Interpreter::visit(const ast::MultiselectHashNode *node)
{
    // evaluate the multiselect hash opration if the context doesn't holds null
    if (!getJsonValue(m_context)->is_null())
    {
        auto doc = hermes::DocView::make_pooled();
        auto result = doc->set_generic_map();

        // move the current context into a temporary variable in case it holds
        // a value, since the context member variable will get overwritten
        // during the evaluation of sub expressions
        ContextValue contextValue {std::move(m_context)};
        // iterate over the list of subexpressions
        for (auto& keyValuePair: node->expressions)
        {
            // assign a const lvalue ref to the context
            m_context = assignContextValue(getJsonValue(contextValue));
            // evaluate the subexpression
            visit(&keyValuePair.second);
            // add a copy of the result of the sub expression as the value
            // for the key of the subexpression
            //result[keyValuePair.first.identifier] = getJsonValue(m_context);

            result->put(keyValuePair.first.identifier, getJsonValue(m_context));
        }
        // set the results of the projection
        m_context = result->as_value();
    }
}

void Interpreter::visit(const ast::NotExpressionNode *node)
{
    // negate the result of the subexpression
    visit(&node->expression);
    m_context = hermes::DocView::wrap_dataobject<Boolean>(!toSimpleBoolean(getJsonValue(m_context)))->as_value();
}

void Interpreter::visit(const ast::ComparatorExpressionNode *node)
{
    using Comparator = ast::ComparatorExpressionNode::Comparator;

    // thow an error if it's an unhandled operator
    if (node->comparator == Comparator::Unknown)
    {
        BOOST_THROW_EXCEPTION(InvalidAgrument{});
    }

    // move the current context into a temporary variable and use a const lvalue
    // reference as the context for evaluating the left side expression, so
    // the context can be reused for the evaluation of the right expression
    ContextValue contextValue {std::move(m_context)};
    m_context = assignContextValue(getJsonValue(contextValue));
    // evaluate the left expression
    visit(&node->leftExpression);


    // move the left side results into a temporary variable
    ContextValue leftResultContext {std::move(m_context)};
    const Json& leftResult = getJsonValue(leftResultContext);

    // set the context for the right side expression
    m_context = std::move(contextValue);
    // evaluate the right expression
    visit(&node->rightExpression);
    const Json& rightResult = getJsonValue(m_context);

    if (node->comparator == Comparator::Equal)
    {
        m_context = wrap_DO<Boolean>(leftResult->equals(rightResult))->as_value();
    }
    else if (node->comparator == Comparator::NotEqual)
    {
        m_context = wrap_DO<Boolean>(!leftResult->equals(rightResult))->as_value();
    }
    else
    {        
        if (node->comparator == Comparator::Less)
        {
            m_context = wrap_DO<Boolean>(leftResult->compare(rightResult) < 0)->as_value();
        }
        else if (node->comparator == Comparator::LessOrEqual)
        {
            m_context = wrap_DO<Boolean>(leftResult->compare(rightResult) <= 0)->as_value();
        }
        else if (node->comparator == Comparator::GreaterOrEqual)
        {
            m_context = wrap_DO<Boolean>(leftResult->compare(rightResult) >= 0)->as_value();
        }
        else if (node->comparator == Comparator::Greater)
        {
            m_context = wrap_DO<Boolean>(leftResult->compare(rightResult) > 0)->as_value();
        }
        else {
            m_context = Json{};
        }
    }
}

void Interpreter::visit(const ast::OrExpressionNode *node)
{
    // evaluate the logic operator and return with the left side result
    // if it's equal to true
    evaluateLogicOperator(node, true);
}

void Interpreter::visit(const ast::AndExpressionNode *node)
{
    // evaluate the logic operator and return with the left side result
    // if it's equal to false
    evaluateLogicOperator(node, false);
}

void Interpreter::evaluateLogicOperator(
        const ast::BinaryExpressionNode* node,
        bool shortCircuitValue)
{
    // move the current context into a temporary variable and use a const lvalue
    // reference as the context for evaluating the left side expression, so
    // the context can be reused for the evaluation of the right expression
    ContextValue contextValue {std::move(m_context)};
    m_context = assignContextValue(getJsonValue(contextValue));
    // evaluate the left expression
    visit(&node->leftExpression);
    // if the left side result is not enough for producing the final result
    if (toSimpleBoolean(getJsonValue(m_context)) != shortCircuitValue)
    {
        // evaluate the right side expression
        m_context = std::move(contextValue);
        visit(&node->rightExpression);
    }
    else
    {
        m_context = contextValue;
    }
}

void Interpreter::visit(const ast::ParenExpressionNode *node)
{
    // evaluate the sub expression
    visit(&node->expression);
}

void Interpreter::visit(const ast::PipeExpressionNode *node)
{
    // evaluate the left followed by the right expression
    visit(&node->leftExpression);
    visit(&node->rightExpression);
}

void Interpreter::visit(const ast::CurrentNode *)
{
}

void Interpreter::visit(const ast::FilterExpressionNode *node)
{
    visit(node, std::move(m_context));
}

template <typename JsonT>
void Interpreter::visit(const ast::FilterExpressionNode* node, JsonT&& context)
{
    // evaluate the filtering operation if the context holds an array
    if (context->is_generic_array())
    {
        auto ctx_array = context->as_generic_array();

        // create the array of results
        auto result = make_array();
        for (auto item: *ctx_array)
        {
            // assign a const lvalue ref of the item to the context
            m_context = assignContextValue(item);
            // evaluate the filtering condition
            visit(&node->expression);
            // convert the result into a boolean
            if (toSimpleBoolean(getJsonValue(m_context))) {
                result->append(item) ;
            }
        }

        // set the results of the projection
        m_context = result->as_value();
    }
    // otherwise evaluate to null
    else
    {
        m_context = {};
    }
}

void Interpreter::visit(const ast::FunctionExpressionNode *node)
{
    // throw an error if the function doesn't exists
    auto it = m_functionMap.find(node->functionName);
    if (it == m_functionMap.end())
    {
        BOOST_THROW_EXCEPTION(UnknownFunction()
                              << InfoFunctionName(node->functionName));
    }

    const auto& descriptor = it->second;
    const auto& argumentArityValidator = std::get<0>(descriptor);
    bool singleContextValueArgument = std::get<1>(descriptor);
    const auto& function = std::get<2>(descriptor);
    // validate that the function has been called with the appropriate
    // number of arguments
    if (!argumentArityValidator(node->arguments.size()))
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentArity());
    }

    // if the function needs more than a single ContextValue
    // argument
    std::shared_ptr<ContextValue> contextValue;
    if (!singleContextValueArgument)
    {
        // move the current context into a temporary variable in
        // case it holds a value
        contextValue = std::make_shared<ContextValue>(std::move(m_context));
    }

    // evaluate the functins arguments
    FunctionArgumentList argumentList = evaluateArguments(
        node->arguments,
        contextValue);
    // evaluate the function
    function(argumentList);
}

void Interpreter::visit(const ast::ExpressionArgumentNode *)
{
}

Index Interpreter::adjustSliceEndpoint(size_t length,
                                        Index endpoint,
                                        Index step) const
{
    if (endpoint < 0)
    {
        endpoint += length;
        if (endpoint < 0)
        {
            endpoint = step < 0 ? -1 : 0;
        }
    }
    else if (endpoint >= length)
    {
        endpoint = step < 0 ? length - 1: length;
    }
    return endpoint;
}

hermes::DataObjectPtr<Boolean> Interpreter::toBoolean(const Json &json) const
{
    return hermes::DocView::wrap_dataobject<Boolean>(
        toSimpleBoolean(json)
    );
}


bool Interpreter::toSimpleBoolean(const Json &json) const
{
    if (json->is_bigint()) {
        return json->as_bigint()->view() != 0;
    }
    else if (json->is_double()) {
        return json->as_double()->view() != 0;
    }
    else if (json->is_varchar()) {
        return json->as_varchar()->view().size() > 0;
    }
    else if (json->is_boolean()) {
        return json->as_boolean()->view();
    }
    else if (json->is_generic_array()) {
        return json->as_generic_array()->size() > 0;
    }
    else if (json->is_generic_map()) {
        return json->as_generic_map()->size() > 0;
    }
    else {
        return false;
    }
}


Interpreter::FunctionArgumentList
Interpreter::evaluateArguments(
    const FunctionExpressionArgumentList &arguments,
    const std::shared_ptr<ContextValue>& contextValue)
{
    // create a list to hold the evaluated expression arguments
    FunctionArgumentList argumentList;
    // evaluate all the arguments and put the results into argumentList
    rng::transform(arguments, std::back_inserter(argumentList),
                   [&, this](const auto& argument)
    {
        // evaluate the current function argument
        auto visitor = boost::hana::overload(
            // evaluate expressions and return their results
            [&, this](const ast::ExpressionNode& node) -> FunctionArgument {
                if (contextValue)
                {
                    const Json& context = getJsonValue(*contextValue);
                    // assign a const lvalue ref to the context
                    m_context = assignContextValue(context);
                }
                // evaluate the expression
                this->visit(&node);
                // move the result
                FunctionArgument result{std::move(m_context)};
                return result;
            },
            // in case of expression argument nodes return the expression they
            // hold so it can be evaluated inside a function
            [](const ast::ExpressionArgumentNode& node) -> FunctionArgument {
                return node.expression;
            },
            // ignore blank arguments
            [](const boost::blank&) -> FunctionArgument {
                return {};
            }
        );
        return boost::apply_visitor(visitor, argument);
    });
    return argumentList;
}

template <typename T>
T& Interpreter::getArgument(FunctionArgument& argument) const
{
    // get a reference to the variable held by the argument
    try
    {
        return boost::get<T&>(argument);
    }
    // or throw an exception if it holds a variable with a different type
    catch (boost::bad_get&)
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
}

const Json &Interpreter::getJsonArgument(FunctionArgument &argument) const
{
    return getJsonValue(getArgument<ContextValue>(argument));
}

void Interpreter::abs(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& value = getJsonArgument(arguments[0]);
    // throw an exception if it's not a number
    if (!value->is_numeric())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // evaluate to either an integer or a float depending on the Json type
    if (value->is_bigint())
    {
        m_context = wrap_DO<BigInt>(std::abs(value->as_bigint()->view()))->as_value();
    }
    else if (value->is_double())
    {
        m_context = wrap_DO<Double>(std::abs(value->as_double()->view()))->as_value();
    }
    else {
        // FIXME: abs() function for...
        m_context = value;
    }
}

void Interpreter::avg(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& items = getJsonArgument(arguments[0]);
    // only evaluate if the argument is an array
    if (items->is_generic_array())
    {
        auto array = items->as_generic_array();

        // evaluate only non empty arrays
        if (!array->empty())
        {
            double itemsSum = 0;
            for (auto item: *array)
            {
                // add the value held by the item to the sum
                if (item->is_bigint())
                {
                    itemsSum += item->as_bigint()->view();
                }
                else if (item->is_double())
                {
                    itemsSum += item->as_double()->view();
                }
                // or throw an exception if the current item is not a number
                else
                {
                    BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
                }
            }
            // the final result is the sum divided by the number of items
            m_context = wrap_DO<Double>(itemsSum / array->size())->as_value();
        }
        // otherwise evaluate to null
        else
        {
            m_context = Json{};
        }
    }
    // otherwise throw an exception
    else
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
}

void Interpreter::contains(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& subject = getJsonArgument(arguments[0]);
    // get the second argument
    const Json& item = getJsonArgument(arguments[1]);
    // throw an exception if the subject item is not an array or a string
    if (!subject->is_generic_array() && !subject->is_varchar())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // evaluate to false by default
    bool result = false;
    // if the subject is an array
    if (subject->is_generic_array())
    {
        auto array = subject->as_generic_array();
        for (auto elem: *array) {
            if (elem->equals(item)) {
                result = true;
            }
        }
    }
    // if the subject is a string
    else if (subject->is_varchar())
    {
        // try to find the given item as a substring in subject
        U8StringView stringSubject  = subject->as_varchar()->view();
        U8StringView stringItem     = item->as_varchar()->view();
        result = boost::contains(stringSubject, stringItem);
    }
    // set the result
    m_context = wrap_DO<Boolean>(result)->as_value();
}

void Interpreter::ceil(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& value = getJsonArgument(arguments[0]);
    // throw an exception if if the value is nto a number
    if (!value->is_numeric())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    // if the value is an integer then it evaluates to itself
    if (value->is_double()) {
        m_context = wrap_DO<Double>(std::ceil(value->as_double()->view()))->as_value();
    }
    else if (value->is_real()) {
        m_context = wrap_DO<Real>(std::ceil(value->as_real()->view()))->as_value();
    }
    else {
        m_context = value;
    }
}

void Interpreter::endsWith(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& subject = getJsonArgument(arguments[0]);
    // get the second argument
    const Json& suffix = getJsonArgument(arguments[1]);
    // throw an exception if the subject or the suffix is not a string
    if (!subject->is_varchar() || !suffix->is_varchar())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // check whether subject ends with the suffix
    auto stringSubject = subject->as_varchar()->view();
    auto stringSuffix = suffix->as_varchar()->view();
    m_context = wrap_DO<Boolean>(boost::ends_with(stringSubject, stringSuffix))->as_value();
}

void Interpreter::floor(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& value = getJsonArgument(arguments[0]);
     // throw an exception if the value is not a number
    if (!value->is_numeric())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    if (value->is_double())
    {
        m_context = wrap_DO<Double>(std::floor(value->as_double()->view()))->as_value();
    }
    else if (value->is_real())
    {
        m_context = wrap_DO<Real>(std::floor(value->as_real()->view()))->as_value();
    }
    else {
        m_context = value;
    }
}

void Interpreter::join(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& glue = getJsonArgument(arguments[0]);
    // get the second argument
    const Json& array_val = getJsonArgument(arguments[1]);
    // throw an exception if the array or glue is not a string or if any items
    // inside the array are not strings
    if (!glue->is_varchar() || !array_val->is_generic_array())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    auto array = array_val->as_generic_array();

    std::vector<String> stringArray;
    for (auto item: *array)
    {
        if (!item->is_varchar()) {
            BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
        }
        else {
            stringArray.emplace_back(item->as_varchar()->view());
        }
    }

    // join together the vector of strings with the glue string
    m_context = wrap_DO<Varchar>(alg::join(stringArray, glue->as_varchar()->view()))->as_value();
}

void Interpreter::keys(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& object = getJsonArgument(arguments[0]);
    // throw an exception if the argument is not an object
    if (!object->is_generic_map())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // add all the keys from the object to the list of results
    auto doc = hermes::DocView::make_pooled();
    auto results = doc->set_generic_array();
    //Json results(Json::value_t::array);
    auto map = object->as_generic_map();

    map->for_each([&](auto k, auto){
        results->append<Varchar>(k);
    });
    // set the result
    m_context = results->as_value();
}

void Interpreter::length(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& subject = getJsonArgument(arguments[0]);
    // throw an exception if the subject item is not an array, object or string
    if (!(subject->is_generic_array() || subject->is_generic_map() || subject->is_varchar()))
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    // if it's a string
    if (subject->is_varchar())
    {
        // calculate the distance between the two unicode iterators
        // (since the expected string encoding is UTF-8 the number of
        // items isn't equals to the number of code points)
        auto stringSubject = subject->as_varchar()->view();
        auto begin = U8UnicodeIteratorAdaptor(std::begin(stringSubject));
        auto end = U8UnicodeIteratorAdaptor(std::end(stringSubject));
        m_context = wrap_DO<BigInt>(std::distance(begin, end))->as_value();
    }
    // otherwise get the size of the array or object
    else if (subject->is_generic_array()) {
        m_context = wrap_DO<BigInt>(subject->as_generic_array()->size())->as_value();
    }
    else {
        m_context = wrap_DO<BigInt>(subject->as_generic_map()->size())->as_value();
    }
}

void Interpreter::map(FunctionArgumentList &arguments)
{
    // get the first argument
    const ast::ExpressionNode& expression
            = getArgument<const ast::ExpressionNode>(arguments[0]);
    // get the second argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[1]);
    map(&expression, std::move(contextValue));
}

template <typename JsonT>
void Interpreter::map(const ast::ExpressionNode* node, JsonT&& array_value)
{
    using std::placeholders::_1;
    // throw an exception if the argument is not an array
    if (!array_value->is_generic_array())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    auto result = make_array();
    auto array = array_value->as_generic_array();
    for (auto item: *array)
    {
        m_context = assignContextValue(std::move(item));
        visit(node);
        result->append(m_context);
    }

    m_context = result->as_value();
}

void Interpreter::merge(FunctionArgumentList &arguments)
{
    using std::placeholders::_1;

    auto doc = hermes::DocView::make_pooled();
    auto result = doc->set_generic_map()->as_value();

    // iterate over the arguments
    for (auto& argument: arguments)
    {
        // convert the argument to a context value
        ContextValue& contextValue = getArgument<ContextValue>(argument);
        const Json& object = getJsonValue(contextValue);
        // throw an exception if it's not an object
        if (!object->is_generic_map())
        {
            BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
        }

        // if the resulting object is still empty then simply assign the first
        // item to it
        if (result->as_generic_map()->empty())
        {
            result = std::move(object);
        }
        // otherwise merge the object into the result object
        else
        {
            mergeObject(&result, std::move(contextValue));
        }
    };
    m_context = std::move(result);
}

template <typename JsonT>
void Interpreter::mergeObject(Json* object, JsonT&& sourceObject)
{
    auto map = (*object)->as_generic_map();
    auto src_map = sourceObject->as_generic_map();

    src_map->for_each([&](auto k, auto v){
        map->put(k, v);
    });
}

void Interpreter::notNull(FunctionArgumentList &arguments)
{
    // iterate over the arguments
    for (auto& argument: arguments)
    {
        // get the current argument
        const Json& item = getJsonArgument(argument);
        // if the current item is not null set it as the result
        if (!item->is_null())
        {
            m_context = item;
            return;
        }
    }
    // if all arguments were null set null as the result
    m_context = {};
}

void Interpreter::reverse(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    reverse(std::move(contextValue));
}

void Interpreter::reverse(Json&& subject)
{

    auto doc = hermes::DocView::make_pooled();
    auto result = doc->set_generic_array();

    auto array = subject->as_generic_array();
    size_t size = array->size();
    for (size_t idx = 0; idx < size; idx++) {
        auto item = array->get(size - idx - 1);
        result->append(item);
    }

    m_context = std::move(result)->as_value();
}

void Interpreter::sort(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    sort(std::move(contextValue));
}

void Interpreter::sort(Json&& array)
{
    std::vector<hermes::ValuePtr> sorted;
    for (auto& item: *(array->as_generic_array())) {
        sorted.push_back(item);
    }

    std::sort(std::begin(sorted), std::end(sorted), [](auto left, auto right){
        return left->compare(right) < 0;
    });

    auto result = wrap_array(sorted);

    m_context = std::move(result)->as_value();
}

void Interpreter::sortBy(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    // get the second argument
    const ast::ExpressionNode& expression
            = getArgument<ast::ExpressionNode>(arguments[1]);

    sortBy(&expression, std::move(contextValue));
}

void Interpreter::sortBy(const ast::ExpressionNode* expression, Json&& source)
{
    using SortT = std::pair<Json, Json>;

    std::vector<SortT> sorted;

    for (auto& item: *source->as_generic_array())
    {
        // visit the mapped expression with the item as the context
        m_context = assignContextValue(item);
        visit(expression);
        const Json& resultValue = getJsonValue(m_context);
        sorted.push_back(SortT{item, resultValue});
    }

    // sort the items of the array based on the results of the expression
    // evaluated on them
    std::sort(std::begin(sorted), std::end(sorted),
              [&](const auto& first, const auto& second) -> bool
    {
        return first.second->compare(second.second) < 0;
    });

    auto doc = hermes::DocView::make_pooled();
    auto result = doc->set_generic_array();

    for (const auto& pair: sorted) {
        result->append(pair.first);
    }

    m_context = std::move(result)->as_value();
}

void Interpreter::startsWith(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& subject = getJsonArgument(arguments[0]);
    // get the second argument
    const Json& prefix = getJsonArgument(arguments[1]);
    // throw an exception if the subject or the prefix is not a string
    if (!subject->is_varchar() || !prefix->is_varchar())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // check whether subject starts with the suffix
    auto stringSubject = subject->as_varchar()->view();
    auto stringPrefix = prefix->as_varchar()->view();
    m_context = wrap_DO<Boolean>(boost::starts_with(stringSubject, stringPrefix))->as_value();
}

void Interpreter::sum(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& items = getJsonArgument(arguments[0]);
    // if the argument is an array
    if (items->is_generic_array())
    {
        auto array = items->as_generic_array();
        double itemsSum{};
        for (auto item: *array)
        {
            if (item->is_bigint())
            {
                itemsSum += item->as_bigint()->view();
            }
            else if (item->is_double())
            {
                itemsSum += item->as_double()->view();
            }
            else
            {
                BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
            }
        }

        // set the result
        m_context = wrap_DO<Double>(itemsSum)->as_value();
    }
    // otherwise throw an exception
    else
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
}

void Interpreter::toArray(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toArray(std::move(contextValue));
}

template <typename JsonT>
void Interpreter::toArray(JsonT&& value)
{
    // evaluate to the argument if it's an array
    if (value->is_generic_array())
    {
        m_context = assignContextValue(std::move(value));
    }
    // otherwise create a single element array with the argument as the only
    // item in it
    else
    {
        auto result = make_array();
        result->append(value);

        m_context = result->as_value();
    }
}

void Interpreter::toString(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toString(std::move(contextValue));
}

template <typename JsonT>
void Interpreter::toString(JsonT&& value)
{
    // evaluate to the argument if it's a string
    if (value->is_varchar())
    {
        m_context = assignContextValue(std::move(value));
    }
    // otherwise convert the value to a string by serializing it
    else
    {
        m_context = wrap_DO<Varchar>(value->to_plain_string())->as_value();
    }
}

void Interpreter::toDouble(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toDouble(std::move(contextValue));
}

template <typename JsonT>
void Interpreter::toDouble(JsonT&& value)
{
    // evaluate to the argument if it's a number
    if (value->is_numeric())
    {
        m_context = assignContextValue(std::move(value));
        return;
    }
    // if it's a string
    else if (value->is_varchar())
    {
        // try to convert the string to a number
        try
        {
            m_context = wrap_DO<Double>(std::stod(String(value->as_varchar()->view())))->as_value();
            return;
        }
        // ignore the possible conversion error and let the default case
        // handle the problem
        catch (std::exception&)
        {
        }
    }
    // otherwise evaluate to null
    m_context = {};
}


void Interpreter::toBigInt(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toBigInt(std::move(contextValue));
}

template <typename JsonT>
void Interpreter::toBigInt(JsonT&& value)
{
    // evaluate to the argument if it's a number
    if (value->is_bigint())
    {
        m_context = assignContextValue(std::move(value));
        return;
    }
    else
    {
        m_context = value->template convert_to<BigInt>()->value();
    }
}


void Interpreter::toBoolean(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toBoolean(std::move(contextValue));
}

template <typename JsonT>
void Interpreter::toBoolean(JsonT&& value)
{
    // evaluate to the argument if it's a number
    if (value->is_bigint())
    {
        m_context = assignContextValue(std::move(value));
        return;
    }
    else
    {
        m_context = value->template convert_to<Boolean>()->value();
    }
}

void Interpreter::type(FunctionArgumentList &arguments)
{
    // get the first argument
    const Json& value = getJsonArgument(arguments[0]);

    if (!value->is_null()) {
        String result = value->type_str();
        m_context = wrap_DO<Varchar>(result)->as_value();
    }
    else {
        m_context = {};
    }
}

void Interpreter::values(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    values(std::move(contextValue));
}

template <typename JsonT>
void Interpreter::values(JsonT&& object)
{
    // throw an exception if the argument is not an object
    if (!object->is_generic_map())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // copy or move all values from object into the list of results based on
    // the type of object argument
    auto result = make_array();
    auto map = object->as_generic_map();

    map->for_each([&](auto, auto val){
        result->append(val);
    });

    m_context = result->as_value();
}

void Interpreter::max(FunctionArgumentList &arguments,
                       const JsonComparator &comparator)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    max(&comparator, std::move(contextValue));
}

template <typename JsonT>
void Interpreter::max(const JsonComparator* comparator, JsonT&& array)
{
    hermes::ValuePtr max_val;

    for (auto item: *array->as_generic_array()) {
        if (max_val->is_not_null()) {
            if (max_val->compare(item) > 0) {
                max_val = item;
            }
        }
        else {
            max_val = item;
        }
    }

    m_context = max_val;
}

void Interpreter::maxBy(FunctionArgumentList &arguments,
                         const JsonComparator &comparator)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    // get the second argument
    const ast::ExpressionNode& expression
            = getArgument<ast::ExpressionNode>(arguments[1]);

    maxBy(&expression, &comparator, std::move(contextValue));
}

template <typename JsonT>
void Interpreter::maxBy(const ast::ExpressionNode* expression,
                         const JsonComparator* comparator,
                         JsonT&& source)
{
    auto array = source->as_generic_array();

    // if the array is not empty
    if (!array->empty())
    {

        using MaxByT = std::pair<Json, Json>;
        std::vector<MaxByT> expressionResults;
        for (auto item: *array)
        {
            // evaluate the expression on the current item
            m_context = assignContextValue(item);
            visit(expression);
            Json result = getJsonValue(m_context);
            expressionResults.push_back(MaxByT{item, result});
        }


        // find the largest item in the vector of results
        auto maxResultsIt = rng::max_element(expressionResults,
                                             [&](const auto& contextLeft,
                                             const auto& contextRight) {
            const Json& left = getJsonValue(contextLeft.second);
            const Json& right = getJsonValue(contextRight.second);
            bool cmp = (*comparator)(left, right);
            return cmp;
        });

        m_context = std::move((*maxResultsIt).first);
    }
    // if it's empty then evaluate to null
    else {
        m_context = {};
    }
}


}} // namespace hermes::path::interpreter
