/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
** Copyright (c) 2022 Victor Smirnov
**
** This file is based on the jmespath.cpp project
** (https://github.com/robertmrk/jmespath.cpp, commitid: 9c9702a)
** and is distributed under the MIT License (MIT).
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
#include "hermes_ast_interpreter.h"
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

namespace {

template <typename DT>
hermes::Object wrap_DO(DTTViewType<DT> view) {
    return hermes::HermesCtr::wrap_dataobject<DT>(view);
}

}

hermes::ObjectArray HermesASTInterpreter::wrap_array(const std::vector<Object>& array) {
    auto doc = hermes::HermesCtr::make_pooled();
    auto arr = doc->make_object_array(array.size());

    for (const auto& item: array) {
        arr = arr.push_back(item);
    }

    return arr;
}

hermes::ObjectArray make_array() {
    auto doc = hermes::HermesCtr::make_pooled();
    auto arr = doc->make_object_array();
    return arr;
}


HermesASTInterpreter::HermesASTInterpreter()
{
    // initialize HermesPath function name to function implementation mapping
    using std::placeholders::_1;
    using std::bind;
    using Descriptor = FunctionDescriptor;
    using FunctionType = void(HermesASTInterpreter::*)(FunctionArgumentList&);
    using MaxFunctionType = void(HermesASTInterpreter::*)(FunctionArgumentList&,
                                                  const PathComparator&);
    auto exactlyOne = bind(std::equal_to<size_t>{}, _1, 1);
    auto exactlyTwo = bind(std::equal_to<size_t>{}, _1, 2);
    auto zeroOrMore = bind(std::greater_equal<size_t>{}, _1, 0);
    auto oneOrMore = bind(std::greater_equal<size_t>{}, _1, 1);
    auto mapPtr = static_cast<FunctionType>(&HermesASTInterpreter::map);
    auto reversePtr = static_cast<FunctionType>(&HermesASTInterpreter::reverse);
    auto sortPtr = static_cast<FunctionType>(&HermesASTInterpreter::sort);
    auto sortByPtr = static_cast<FunctionType>(&HermesASTInterpreter::sortBy);
    auto toArrayPtr = static_cast<FunctionType>(&HermesASTInterpreter::toArray);
    auto toStringPtr = static_cast<FunctionType>(&HermesASTInterpreter::toString);
    auto toDoublePtr = static_cast<FunctionType>(&HermesASTInterpreter::toDouble);
    auto toBigIntPtr = static_cast<FunctionType>(&HermesASTInterpreter::toBigInt);
    auto toBooleanPtr = static_cast<FunctionType>(&HermesASTInterpreter::toBoolean);
    auto valuesPtr = static_cast<FunctionType>(&HermesASTInterpreter::values);
    auto maxPtr = static_cast<MaxFunctionType>(&HermesASTInterpreter::max);
    auto maxByPtr = static_cast<MaxFunctionType>(&HermesASTInterpreter::maxBy);
    m_functionMap = {
        {"abs", Descriptor{exactlyOne, true,
                           bind(&HermesASTInterpreter::abs, this, _1)}},
        {"avg",  Descriptor{exactlyOne, true,
                            bind(&HermesASTInterpreter::avg, this, _1)}},
        {"contains", Descriptor{exactlyTwo, false,
                                bind(&HermesASTInterpreter::contains, this, _1)}},
        {"ceil", Descriptor{exactlyOne, true,
                            bind(&HermesASTInterpreter::ceil, this, _1)}},
        {"ends_with", Descriptor{exactlyTwo, false,
                                 bind(&HermesASTInterpreter::endsWith, this, _1)}},
        {"floor", Descriptor{exactlyOne, true,
                             bind(&HermesASTInterpreter::floor, this, _1)}},
        {"join", Descriptor{exactlyTwo, false,
                            bind(&HermesASTInterpreter::join, this, _1)}},
        {"keys", Descriptor{exactlyOne, true,
                            bind(&HermesASTInterpreter::keys, this, _1)}},
        {"length", Descriptor{exactlyOne, true,
                              bind(&HermesASTInterpreter::length, this, _1)}},
        {"map", Descriptor{exactlyTwo, true,
                           bind(mapPtr, this, _1)}},
        {"max", Descriptor{exactlyOne, true,
                           bind(maxPtr, this, _1, hermes::Less{})}},
        {"max_by", Descriptor{exactlyTwo, true,
                              bind(maxByPtr, this, _1, hermes::Less{})}},
        {"merge", Descriptor{zeroOrMore, false,
                             bind(&HermesASTInterpreter::merge, this, _1)}},
        {"min", Descriptor{exactlyOne, true,
                           bind(maxPtr, this, _1, hermes::Greater{})}},
        {"min_by", Descriptor{exactlyTwo, true,
                              bind(maxByPtr, this, _1, hermes::Greater{})}},
        {"not_null", Descriptor{oneOrMore, false,
                                bind(&HermesASTInterpreter::notNull, this, _1)}},
        {"reverse", Descriptor{exactlyOne, true,
                               bind(reversePtr, this, _1)}},
        {"sort",  Descriptor{exactlyOne, true,
                             bind(sortPtr, this, _1)}},
        {"sort_by", Descriptor{exactlyTwo, true,
                               bind(sortByPtr, this, _1)}},
        {"starts_with", Descriptor{exactlyTwo, false,
                                   bind(&HermesASTInterpreter::startsWith, this, _1)}},
        {"sum", Descriptor{exactlyOne, true,
                           bind(&HermesASTInterpreter::sum, this, _1)}},
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
                            bind(&HermesASTInterpreter::type, this, _1)}},
        {"values", Descriptor{exactlyOne, true,
                              bind(valuesPtr, this, _1)}}
    };
}

void HermesASTInterpreter::evaluateProjection(const ASTNodePtr& expression)
{
    using std::placeholders::_1;
    // move the current context into a temporary variable in case it holds
    // a value, since the context member variable will get overwritten during
    // the evaluation of the projection
    evaluateProjection(expression, std::move(m_context));
}


void HermesASTInterpreter::evaluateProjection(
        const ASTNodePtr& expression,
        ContextValue&& icontext
) {
    using std::placeholders::_1;
    // evaluate the projection if the context holds an array

    if (is_hermes_object(icontext))
    {
        auto context = getPathObject(icontext);
        if (context.is_array())
        {
            auto ctx_array = context.as_generic_array();

            // create the array of results
            auto result = make_array();

            // iterate over the array
            for (size_t idx = 0; idx < ctx_array->size(); idx++)
            {
                auto item = ctx_array->get(idx);
                // move the item into the context or create an lvalue reference
                // depending on the type of the context variable
                m_context = assignContextValue(std::move(item));
                // evaluate the expression
                visit(expression);
                // if the result of the expression is not null
                if (!getPathObject(m_context).is_null())
                {
                    // add the result of the expression to the results array
                    result = result.push_back(getPathObject(m_context));
                }
            }

            // set the results of the projection
            m_context = result.as_object();
        }
        // otherwise evaluate to null
        else {
            m_context = {};
        }
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Projection is not supported on this non-hermes value").do_throw();
    }
}

void HermesASTInterpreter::visitNullNode(const ASTNodePtr& node) {}


void HermesASTInterpreter::visitIdentifierNode(const ASTNodePtr& node)
{
    visitIdentifierNode2(node, std::move(m_context));
}


void HermesASTInterpreter::visitIdentifierNode2(const ASTNodePtr& node, ContextValue&& icontext)
{
    auto prop_name = node.get(IDENTIFIER_ATTR);

    // evaluate the current function argument
    auto visitor = boost::hana::overload(
        // evaluate expressions and return their results
        [&](const HermesObjectResolver* resolver) -> Object {
            auto name = prop_name.as_varchar();
            return (*resolver)(name);
        },
        // ignore blank arguments
        [&](const Object& value) -> Object {

            // evaluete the identifier if the context holds an object
            if (value.is_not_null() && value.is_map())
            {
                auto map = value.as_generic_map();
                return map->get(prop_name);
            }

            return {};
        }
    );
    m_context = boost::apply_visitor(visitor, icontext);
}

void HermesASTInterpreter::visitRawStringNode(const ASTNodePtr& node)
{
    m_context = expect_attr(node, RAW_STRING_ATTR);
}

void HermesASTInterpreter::visitHermesValueNode(const ASTNodePtr& node)
{
    auto value = node.get(VALUE_ATTR);

    if (value.is_a(TypeTag<Parameter>()))
    {
        if (parameter_resolver_) {
            Parameter param = value.cast_to<Parameter>();
            if (parameter_resolver_->has_parameter(param.view())) {
                m_context = parameter_resolver_->resolve(param.view());
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("Parameter {} resolution failure", param.view()).do_throw();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes Parameter resolver is not configured for Path expression").do_throw();
        }
    }
    else {
        m_context = value;
    }
}

void HermesASTInterpreter::visitSubexpressionNode(const ASTNodePtr& node)
{
    visit(node.get(LEFT_EXPRESSION_ATTR));
    visit(node.get(RIGHT_EXPRESSION_ATTR));
}

void HermesASTInterpreter::visitIndexExpressionNode(const ASTNodePtr& node)
{
    // evaluate the left side expression
    visit(node.get(LEFT_EXPRESSION_ATTR));
    // evaluate the index expression if the context holds an array
    if (getPathObject(m_context).is_array())
    {
        // evaluate the bracket specifier
        visit(node.get(BRACKET_SPECIFIER_ATTR));
        // if the index expression also defines a projection then evaluate it
        auto pattr = node.get(IS_PROJECTION_ATTR);
        if (pattr.to_bool()) {
            evaluateProjection(node.get(RIGHT_EXPRESSION_ATTR).as_tiny_object_map());
        }
    }
    // otherwise evaluate to null
    else
    {
        m_context = {};
    }
}

void HermesASTInterpreter::visitArrayItemNode(const ASTNodePtr& node)
{
    visitArrayItemNode2(node, std::move(m_context));
}


void HermesASTInterpreter::visitArrayItemNode2(const ASTNodePtr& node, ContextValue&& icontext)
{
    auto context = getPathObject(icontext);

    // evaluate the array item expression if the context holds an array
    if (context.is_array())
    {
        auto arr = context.as_generic_array();
        // normalize the index value
        auto arrayIndex = node.get(INDEX_ATTR).to_i64();
        if (arrayIndex < 0) {
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

void HermesASTInterpreter::visitFlattenOperatorNode(const ASTNodePtr& node)
{
    visitFlattenOperatorNode2(node, std::move(m_context));
}


void HermesASTInterpreter::visitFlattenOperatorNode2(const ASTNodePtr& node, ContextValue&& icontext)
{
    auto context = getPathObject(icontext);

    // evaluate the flatten operation if the context holds an array
    if (context.is_array())
    {
        auto ctx_array = context.as_generic_array();
        auto result = make_array();

        // iterate over the array
        for (size_t idx = 0; idx < ctx_array->size(); idx++)
        {
            auto item = ctx_array->get(idx);

            // if the item is an array append or move every one of its items
            // to the end of the results variable
            if (item.is_array())
            {
                auto a0 = item.as_generic_array();
                for (size_t idx = 0; idx < a0->size(); idx++)
                {
                    auto item2 = a0->get(idx);
                    result = result.push_back(item2);
                }
            }
            // otherwise append or move the item
            else
            {
                result = result.push_back(std::move(item));
            }
        }
        // set the results of the flatten operation
        m_context = result.as_object();
    }
    // otherwise evaluate to null
    else
    {
        m_context = {};
    }
}


void HermesASTInterpreter::visitSliceExpressionNode(const ASTNodePtr& node)
{
    visitSliceExpressionNode2(node, std::move(m_context));
}


void HermesASTInterpreter::visitSliceExpressionNode2(const ASTNodePtr& node, ContextValue&& icontext)
{
    auto context = getPathObject(icontext);

    // evaluate the slice operation if the context holds an array
    if (context.is_array())
    {
        auto ctx_array = context.as_generic_array();

        int64_t startIndex = 0;
        int64_t stopIndex = 0;
        int64_t step = 1;
        size_t length = ctx_array->size();

        auto node_step = node.get(STEP_ATTR);

        // verify the validity of slice indeces and normalize their values
        if (node_step.is_not_null())
        {
            auto val = node_step.to_i64();
            if (val == 0)
            {
                BOOST_THROW_EXCEPTION(InvalidValue{});
            }
            step = val;
        }

        auto node_start = node.get(START_ATTR);
        auto node_stop  = node.get(STOP_ATTR);

        if (node_start.is_null())
        {
            startIndex = step < 0 ? length - 1: 0;
        }
        else
        {
            auto val = node_start.to_i64();
            startIndex = adjustSliceEndpoint(length, val, step);
        }

        if (node_stop.is_null())
        {
            stopIndex = step < 0 ? -1 : length;
        }
        else
        {
            auto val = node_stop.to_i64();
            stopIndex = adjustSliceEndpoint(length, val, step);
        }

        // create the array of results
        auto result = make_array();

        // iterate over the array
        for (auto i = startIndex;
             step > 0 ? (i < stopIndex) : (i > stopIndex);
             i += step)
        {
            // append a copy of the item at arrayIndex or move it into the
            // result array depending on the type of the context variable
            size_t arrayIndex = static_cast<uint64_t>(i);
            result = result.push_back(std::move(ctx_array->get(arrayIndex)));
        }

        // set the results of the projection
        m_context = result.as_object();
    }
    // otherwise evaluate to null
    else {
        m_context = {};
    }
}

void HermesASTInterpreter::visitListWildcardNode(const ASTNodePtr& node)
{
    // evaluate a list wildcard operation to null if the context isn't an array
    if (!getPathObject(m_context).is_array()) {
        m_context = {};
    }
}

void HermesASTInterpreter::visitHashWildcardNode(const ASTNodePtr& node)
{
    visitHashWildcardNode2(node, std::move(m_context));
}


void HermesASTInterpreter::visitHashWildcardNode2(const ASTNodePtr& node, ContextValue&& icontext)
{
    auto context = getPathObject(icontext);

    // evaluate the hash wildcard operation if the context holds an array
    if (context.is_map())
    {
        auto ctx_map = context.as_generic_map();

        auto result = make_array();

        ctx_map->for_each([&](auto, auto value){
            result = result.push_back(value);
        });

        // set the results of the projection
        m_context = result.as_object();
    }
    // otherwise evaluate to null
    else {
        m_context = {};
    }
    // evaluate the projected sub expression
    evaluateProjection(node.get(RIGHT_EXPRESSION_ATTR).as_tiny_object_map());
}

void HermesASTInterpreter::visitMultiselectListNode(const ASTNodePtr& node)
{
    // evaluate the multiselect list opration if the context doesn't holds null
    if (!getPathObject(m_context).is_null())
    {
        auto result = make_array();

        // move the current context into a temporary variable in case it holds
        // a value, since the context member variable will get overwritten
        // during  the evaluation of sub expressions
        ContextValue contextValue {std::move(m_context)};
        // iterate over the list of subexpressions
        ObjectArray expressions = node.get(EXPRESSIONS_ATTR).as_object_array();
        for (size_t c = 0; c < expressions.size(); c++)
        {
            Object expression = expressions.get(c);

            // assign a const lvalue ref to the context
            m_context = assignContextValue(getPathObject(contextValue));
            // evaluate the subexpression
            visit(expression.as_tiny_object_map());
            // copy the result of the subexpression into the list of results
            result = result.push_back(getPathObject(m_context));
        }
        // set the results of the projection
        m_context = result.as_object();
    }
}

void HermesASTInterpreter::visitMultiselectHashNode(const ASTNodePtr& node)
{
    // evaluate the multiselect hash opration if the context doesn't holds null
    if (!getPathObject(m_context).is_null())
    {
        auto doc = hermes::HermesCtr::make_pooled();
        auto result = doc->make_object_map();
        doc->set_root(result.as_object());

        // move the current context into a temporary variable in case it holds
        // a value, since the context member variable will get overwritten
        // during the evaluation of sub expressions
        ContextValue contextValue {std::move(m_context)};
        // iterate over the list of subexpressions

        ObjectArray expressions = node.get(EXPRESSIONS_ATTR).as_object_array();
        for (size_t c = 0; c < expressions.size(); c++)
        {
            Object keyValuePairObj = expressions.get(c);
            // assign a const lvalue ref to the context
            m_context = assignContextValue(getPathObject(contextValue));

            // evaluate the subexpression
            auto keyValuePair = keyValuePairObj.as_tiny_object_map();
            visit(keyValuePair.get(SECOND_ATTR));
            // add a copy of the result of the sub expression as the value
            // for the key of the subexpression
            //result[keyValuePair.first.identifier] = getJsonValue(m_context);

            result = result.put(
                keyValuePair.get(FIRST_ATTR).as_tiny_object_map().get(IDENTIFIER_ATTR).as_varchar(),
                getPathObject(m_context)
            );
        }
        // set the results of the projection
        m_context = result.as_object();
    }
}

void HermesASTInterpreter::visitNotExpressionNode(const ASTNodePtr& node)
{
    // negate the result of the subexpression
    visit(node.get(EXPRESSION_ATTR).as_tiny_object_map());
    m_context = hermes::HermesCtr::wrap_dataobject<Boolean>(!toSimpleBoolean(getPathObject(m_context))).as_object();
}

void HermesASTInterpreter::visitComparatorExpressionNode(const ASTNodePtr& node)
{
    using Comparator = ast::ComparatorExpressionNode::Comparator;

    Comparator comparator = (Comparator)node.get(COMPARATOR_ATTR).to_i64();

    // thow an error if it's an unhandled operator
    if (comparator == Comparator::Unknown)
    {
        BOOST_THROW_EXCEPTION(InvalidAgrument{});
    }

    // move the current context into a temporary variable and use a const lvalue
    // reference as the context for evaluating the left side expression, so
    // the context can be reused for the evaluation of the right expression
    ContextValue contextValue {std::move(m_context)};
    m_context = assignContextValue(getPathObject(contextValue));

    // evaluate the left expression
    visit(node.get(LEFT_EXPRESSION_ATTR));

    // move the left side results into a temporary variable
    ContextValue leftResultContext {std::move(m_context)};
    const Object& leftResult = getPathObject(leftResultContext);

    // set the context for the right side expression
    m_context = std::move(contextValue);
    // evaluate the right expression
    visit(node.get(RIGHT_EXPRESSION_ATTR));
    const Object& rightResult = getPathObject(m_context);

    if (comparator == Comparator::Equal)
    {
        m_context = wrap_DO<Boolean>(leftResult.equals(rightResult)).as_object();
    }
    else if (comparator == Comparator::NotEqual)
    {
        m_context = wrap_DO<Boolean>(!leftResult.equals(rightResult)).as_object();
    }
    else
    {        
        if (comparator == Comparator::Less)
        {
            m_context = wrap_DO<Boolean>(leftResult.compare(rightResult) < 0).as_object();
        }
        else if (comparator == Comparator::LessOrEqual)
        {
            m_context = wrap_DO<Boolean>(leftResult.compare(rightResult) <= 0).as_object();
        }
        else if (comparator == Comparator::GreaterOrEqual)
        {
            m_context = wrap_DO<Boolean>(leftResult.compare(rightResult) >= 0).as_object();
        }
        else if (comparator == Comparator::Greater)
        {
            m_context = wrap_DO<Boolean>(leftResult.compare(rightResult) > 0).as_object();
        }
        else {
            m_context = Object{};
        }
    }
}

void HermesASTInterpreter::visitOrExpressionNode(const ASTNodePtr& node)
{
    // evaluate the logic operator and return with the left side result
    // if it's equal to true
    evaluateLogicOperator(node, true);
}

void HermesASTInterpreter::visitAndExpressionNode(const ASTNodePtr& node)
{
    // evaluate the logic operator and return with the left side result
    // if it's equal to false
    evaluateLogicOperator(node, false);
}

void HermesASTInterpreter::evaluateLogicOperator(
        const ASTNodePtr& node,
        bool shortCircuitValue
){
    // move the current context into a temporary variable and use a const lvalue
    // reference as the context for evaluating the left side expression, so
    // the context can be reused for the evaluation of the right expression
    ContextValue contextValue {std::move(m_context)};
    m_context = assignContextValue(getPathObject(contextValue));
    // evaluate the left expression
    visit(node.get(LEFT_EXPRESSION_ATTR));
    // if the left side result is not enough for producing the final result
    if (toSimpleBoolean(getPathObject(m_context)) != shortCircuitValue)
    {
        // evaluate the right side expression
        m_context = std::move(contextValue);
        visit(node.get(RIGHT_EXPRESSION_ATTR));
    }
    else
    {
        m_context = contextValue;
    }
}

void HermesASTInterpreter::visitParenExpressionNode(const ASTNodePtr& node)
{
    // evaluate the sub expression
    visit(node.get(EXPRESSION_ATTR));
}

void HermesASTInterpreter::visitPipeExpressionNode(const ASTNodePtr& node)
{
    // evaluate the left followed by the right expression
    visit(node.get(LEFT_EXPRESSION_ATTR));
    visit(node.get(RIGHT_EXPRESSION_ATTR));
}

void HermesASTInterpreter::visitCurrentNode(const ASTNodePtr& node)
{

}

void HermesASTInterpreter::visitFilterExpressionNode(const ASTNodePtr& node)
{
    visitFilterExpressionNode2(node, std::move(m_context));
}


void HermesASTInterpreter::visitFilterExpressionNode2(const ASTNodePtr& node, ContextValue&& icontext)
{
    auto context = getPathObject(icontext);

    // evaluate the filtering operation if the context holds an array
    if (context.is_array())
    {
        auto ctx_array = context.as_generic_array();

        // create the array of results
        auto result = make_array();
        for (size_t idx = 0; idx < ctx_array->size(); idx++)
        {
            auto item = ctx_array->get(idx);

            // assign a const lvalue ref of the item to the context
            m_context = assignContextValue(item);
            // evaluate the filtering condition
            visit(node.get(EXPRESSION_ATTR));
            // convert the result into a boolean
            if (toSimpleBoolean(getPathObject(m_context))) {
                result = result.push_back(item) ;
            }
        }

        // set the results of the projection
        m_context = result.as_object();
    }
    // otherwise evaluate to null
    else
    {
        m_context = {};
    }
}

void HermesASTInterpreter::visitFunctionExpressionNode(const ASTNodePtr& node)
{
    // throw an error if the function doesn't exists
    U8String fname = node.get(FUNCTION_NAME_ATTR).to_str();
    auto it = m_functionMap.find(fname.to_std_string());
    if (it == m_functionMap.end())
    {
        BOOST_THROW_EXCEPTION(UnknownFunction()
                              << InfoFunctionName(fname.to_std_string()));
    }

    const auto& descriptor = it->second;
    const auto& argumentArityValidator = std::get<0>(descriptor);
    bool singleContextValueArgument = std::get<1>(descriptor);
    const auto& function = std::get<2>(descriptor);
    // validate that the function has been called with the appropriate
    // number of arguments

    ObjectArray arguments = node.get(ARGUMENTS_ATTR).as_object_array();

    if (!argumentArityValidator(arguments.size())) {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentArity());
    }

    // if the function needs more than a single ContextValue
    // argument
    ContextValue contextValue;
    if (!singleContextValueArgument)
    {
        // move the current context into a temporary variable in
        // case it holds a value
        contextValue = std::move(m_context);
    }

    // evaluate the functins arguments
    FunctionArgumentList argumentList = evaluateArguments(
        arguments,
        contextValue);
    // evaluate the function
    function(argumentList);
}

void HermesASTInterpreter::visitExpressionArgumentNode(const ASTNodePtr& node)
{
}

int64_t HermesASTInterpreter::adjustSliceEndpoint(size_t length,
                                        int64_t endpoint,
                                        int64_t step) const
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

hermes::Object HermesASTInterpreter::toBoolean(const Object &path) const
{
    return hermes::HermesCtr::wrap_dataobject<Boolean>(
        toSimpleBoolean(path)
    );
}


bool HermesASTInterpreter::toSimpleBoolean(const Object &path)
{
    if (path.is_null()) {
        return false;
    }
    if (path.is_varchar()) {
        return path.as_varchar().size() > 0;
    }
    else if (path.is_array()) {
        return path.as_generic_array()->size() > 0;
    }
    else if (path.is_map()) {
        return path.as_generic_map()->size() > 0;
    }
    else if (path.is_convertible_to<Boolean>()) {
        return path.to_bool();
    }
    else {
        return false;
    }
}


HermesASTInterpreter::FunctionArgumentList
HermesASTInterpreter::evaluateArguments(
    const ObjectArray &arguments,
    const ContextValue& icontextValue)
{
    auto contextValue = getPathObject(icontextValue);

    // create a list to hold the evaluated expression arguments
    FunctionArgumentList argumentList;
    // evaluate all the arguments and put the results into argumentList

    for (size_t c = 0; c < arguments.size(); c++)
    {
        Object argument = arguments.get(c);
        if (argument.is_map())
        {
            ASTNodePtr map = argument.as_tiny_object_map();
            Object code_attr = map.get(CODE_ATTR);
            if (code_attr.is_not_null())
            {
                NamedCode code = code_attr.to_i64();
                if (code == ast::ExpressionArgumentNode::CODE) {
                    argumentList.push_back(map.get(EXPRESSION_ATTR).as_tiny_object_map());
                }
                else {
                    if (contextValue.is_not_null())
                    {
                        const Object& context = getPathObject(contextValue);
                        // assign a const lvalue ref to the context
                        m_context = assignContextValue(context);
                    }
                    // evaluate the expression
                    this->visit(map);
                    // move the result
                    FunctionArgument result{std::move(m_context)};
                    argumentList.push_back(result);
                }
            }
            else {
                argumentList.push_back(ContextValue{argument});
            }
        }
        else {
            argumentList.push_back(ContextValue{argument});
        }
    }
    return argumentList;
}

template <typename T>
T& HermesASTInterpreter::getArgument(FunctionArgument& argument) const
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

const Object &HermesASTInterpreter::getArgument(FunctionArgument &argument) const
{
    return getPathObject(getArgument<ContextValue>(argument));
}

void HermesASTInterpreter::abs(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& value = getArgument(arguments[0]);
    // throw an exception if it's not a number
    if (!value.is_numeric())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // evaluate to either an integer or a float depending on the Object type
    if (value.is_bigint())
    {
        m_context = wrap_DO<BigInt>(std::abs(value.to_i64())).as_object();
    }
    else {
        m_context = wrap_DO<Double>(std::abs(value.to_d64())).as_object();
    }
}

void HermesASTInterpreter::avg(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& items = getArgument(arguments[0]);
    // only evaluate if the argument is an array
    if (items.is_array())
    {
        auto array = items.as_generic_array();

        // evaluate only non empty arrays
        if (array->size())
        {
            double itemsSum = 0;
            for (size_t idx = 0; idx < array->size(); idx++)
            {
                auto item = array->get(idx);

                // add the value held by the item to the sum
                if (item.is_bigint())
                {
                    itemsSum += item.as_bigint();
                }
                else if (item.is_double())
                {
                    itemsSum += item.as_double();
                }
                // or throw an exception if the current item is not a number
                else
                {
                    BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
                }
            }
            // the final result is the sum divided by the number of items
            m_context = wrap_DO<Double>(itemsSum / array->size()).as_object();
        }
        // otherwise evaluate to null
        else
        {
            m_context = Object{};
        }
    }
    // otherwise throw an exception
    else
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
}

void HermesASTInterpreter::contains(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& subject = getArgument(arguments[0]);
    // get the second argument
    const Object& item = getArgument(arguments[1]);
    // throw an exception if the subject item is not an array or a string
    if (!subject.is_array() && !subject.is_varchar())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // evaluate to false by default
    bool result = false;
    // if the subject is an array
    if (subject.is_array())
    {
        auto array = subject.as_generic_array();
        for (size_t idx = 0; idx < array->size(); idx++)
        {
            auto elem = array->get(idx);
            if (elem.equals(item)) {
                result = true;
            }
        }
    }
    // if the subject is a string
    else if (subject.is_varchar())
    {
        // try to find the given item as a substring in subject
        U8StringView stringSubject  = subject.as_varchar();
        U8StringView stringItem     = item.as_varchar();
        result = boost::contains(stringSubject, stringItem);
    }
    // set the result
    m_context = wrap_DO<Boolean>(result).as_object();
}

void HermesASTInterpreter::ceil(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& value = getArgument(arguments[0]);
    // throw an exception if if the value is nto a number
    if (!value.is_numeric())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    // if the value is an integer then it evaluates to itself
    if (value.is_double()) {
        m_context = wrap_DO<Double>(std::ceil(value.as_double())).as_object();
    }
    else if (value.is_real()) {
        m_context = wrap_DO<Real>(std::ceil(value.as_real())).as_object();
    }
    else {
        m_context = value;
    }
}

void HermesASTInterpreter::endsWith(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& subject = getArgument(arguments[0]);
    // get the second argument
    const Object& suffix = getArgument(arguments[1]);
    // throw an exception if the subject or the suffix is not a string
    if (!subject.is_varchar() || !suffix.is_varchar())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // check whether subject ends with the suffix
    auto stringSubject = subject.as_varchar();
    auto stringSuffix = suffix.as_varchar();
    m_context = wrap_DO<Boolean>(boost::ends_with(stringSubject, stringSuffix)).as_object();
}

void HermesASTInterpreter::floor(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& value = getArgument(arguments[0]);
     // throw an exception if the value is not a number
    if (!value.is_numeric())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    if (value.is_double())
    {
        m_context = wrap_DO<Double>(std::floor(value.to_d64())).as_object();
    }
    else if (value.is_real())
    {
        m_context = wrap_DO<Real>(std::floor(value.to_f32())).as_object();
    }
    else {
        m_context = value;
    }
}

void HermesASTInterpreter::join(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& glue = getArgument(arguments[0]);
    // get the second argument
    const Object& array_val = getArgument(arguments[1]);
    // throw an exception if the array or glue is not a string or if any items
    // inside the array are not strings
    if (!glue.is_varchar() || !array_val.is_array())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    auto array = array_val.as_generic_array();

    std::vector<String> stringArray;
    for (size_t idx = 0; idx < array->size(); idx++)
    {
        auto item = array->get(idx);
        if (!item.is_varchar()) {
            BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
        }
        else {
            stringArray.emplace_back(item.as_varchar());
        }
    }

    // join together the vector of strings with the glue string
    m_context = wrap_DO<Varchar>(alg::join(stringArray, glue.as_varchar())).as_object();
}

void HermesASTInterpreter::keys(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& object = getArgument(arguments[0]);
    // throw an exception if the argument is not an object
    if (!object.is_map())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // add all the keys from the object to the list of results
    auto doc = hermes::HermesCtr::make_pooled();
    auto results = doc->make_object_array();
    doc->set_root(results.as_object());
    //Object results(Object::value_t::array);
    auto map = object.as_generic_map();

    map->for_each([&](auto k, auto){
        results = results.push_back_t<Varchar>(k.as_varchar());
    });
    // set the result
    m_context = results.as_object();
}

void HermesASTInterpreter::length(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& subject = getArgument(arguments[0]);
    // throw an exception if the subject item is not an array, object or string
    if (!(subject.is_array() || subject.is_map() || subject.is_varchar()))
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    // if it's a string
    if (subject.is_varchar())
    {
        // calculate the distance between the two unicode iterators
        // (since the expected string encoding is UTF-8 the number of
        // items isn't equals to the number of code points)
        auto stringSubject = subject.as_varchar();
        auto begin = U8UnicodeIteratorAdaptor(std::begin(stringSubject));
        auto end = U8UnicodeIteratorAdaptor(std::end(stringSubject));
        m_context = wrap_DO<BigInt>(std::distance(begin, end)).as_object();
    }
    // otherwise get the size of the array or object
    else if (subject.is_array()) {
        m_context = wrap_DO<BigInt>(subject.as_generic_array()->size()).as_object();
    }
    else {
        m_context = wrap_DO<BigInt>(subject.as_generic_map()->size()).as_object();
    }
}

void HermesASTInterpreter::map(FunctionArgumentList &arguments)
{
    // get the first argument
    const ASTNodePtr& expression
            = getArgument<const ASTNodePtr>(arguments[0]);
    // get the second argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[1]);
    map(expression, std::move(contextValue));
}


void HermesASTInterpreter::map(const ASTNodePtr& node, ContextValue&& iarray_value)
{
    auto array_value = getPathObject(iarray_value);

    using std::placeholders::_1;
    // throw an exception if the argument is not an array
    if (!array_value.is_array())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }

    auto result = make_array();
    auto array = array_value.as_generic_array();
    for (size_t idx = 0; idx < array->size(); idx++)
    {
        auto item = array->get(idx);
        m_context = assignContextValue(std::move(item));
        visit(node);
        result = result.push_back(getPathObject(m_context));
    }

    m_context = result.as_object();
}

void HermesASTInterpreter::merge(FunctionArgumentList &arguments)
{
    using std::placeholders::_1;

    auto doc = hermes::HermesCtr::make_pooled();
    auto result = doc->make_object_map().as_object();
    doc->set_root(result.as_object());

    // iterate over the arguments
    for (auto& argument: arguments)
    {
        // convert the argument to a context value
        ContextValue& contextValue = getArgument<ContextValue>(argument);
        const Object& object = getPathObject(contextValue);
        // throw an exception if it's not an object
        if (!object.is_map())
        {
            BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
        }

        // if the resulting object is still empty then simply assign the first
        // item to it
        if (result.as_generic_map()->empty())
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


void HermesASTInterpreter::mergeObject(Object* object, ContextValue&& isourceObject)
{
    auto sourceObject = getPathObject(isourceObject);

    auto map = object->as_generic_map();
    auto src_map = sourceObject.as_generic_map();

    src_map->for_each([&](auto k, auto v){
        map = map->put(k, v);
    });
}

void HermesASTInterpreter::notNull(FunctionArgumentList &arguments)
{
    // iterate over the arguments
    for (auto& argument: arguments)
    {
        // get the current argument
        const Object& item = getArgument(argument);
        // if the current item is not null set it as the result
        if (!item.is_null())
        {
            m_context = item;
            return;
        }
    }
    // if all arguments were null set null as the result
    m_context = {};
}

void HermesASTInterpreter::reverse(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    reverse(std::move(contextValue));
}

void HermesASTInterpreter::reverse(ContextValue&& isubject)
{
    auto subject = getPathObject(isubject);

    auto doc = hermes::HermesCtr::make_pooled();
    auto result = doc->make_object_array();
    doc->set_root(result.as_object());

    auto array = subject.as_generic_array();
    size_t size = array->size();
    for (size_t idx = 0; idx < size; idx++) {
        auto item = array->get(size - idx - 1);
        result = result.push_back(item);
    }

    m_context = std::move(result).as_object();
}

void HermesASTInterpreter::sort(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    sort(std::move(contextValue));
}

void HermesASTInterpreter::sort(ContextValue&& iarray)
{
    auto array = getPathObject(iarray);

    std::vector<hermes::Object> sorted;
    auto garray = array.as_generic_array();
    for (size_t idx = 0; idx < garray->size(); idx++)
    {
        auto item = garray->get(idx);
        sorted.push_back(item);
    }

    std::sort(std::begin(sorted), std::end(sorted), [](auto left, auto right){
        return left.compare(right) < 0;
    });

    auto result = wrap_array(sorted);

    m_context = std::move(result).as_object();
}

void HermesASTInterpreter::sortBy(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    // get the second argument
    const ASTNodePtr& expression
            = getArgument<ASTNodePtr>(arguments[1]);

    sortBy(expression, std::move(contextValue));
}

void HermesASTInterpreter::sortBy(const ASTNodePtr& expression, ContextValue&& isource)
{
    auto source = getPathObject(isource);

    using SortT = std::pair<Object, Object>;

    std::vector<SortT> sorted;

    auto array = source.as_generic_array();
    for (size_t idx = 0; idx < array->size(); idx++)
    {
        auto item = array->get(idx);
        // visit the mapped expression with the item as the context
        m_context = assignContextValue(item);
        visit(expression);
        const Object& resultValue = getPathObject(m_context);
        sorted.push_back(SortT{item, resultValue});
    }

    // sort the items of the array based on the results of the expression
    // evaluated on them
    std::sort(std::begin(sorted), std::end(sorted),
              [&](const auto& first, const auto& second) -> bool
    {
        return first.second.compare(second.second) < 0;
    });

    auto result = make_array();
    for (const auto& pair: sorted) {
        result = result.push_back(pair.first);
    }

    m_context = std::move(result).as_object();
}

void HermesASTInterpreter::startsWith(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& subject = getArgument(arguments[0]);
    // get the second argument
    const Object& prefix = getArgument(arguments[1]);
    // throw an exception if the subject or the prefix is not a string
    if (!subject.is_varchar() || !prefix.is_varchar())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // check whether subject starts with the suffix
    auto stringSubject = subject.as_varchar();
    auto stringPrefix = prefix.as_varchar();
    m_context = wrap_DO<Boolean>(boost::starts_with(stringSubject, stringPrefix)).as_object();
}

void HermesASTInterpreter::sum(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& items = getArgument(arguments[0]);
    // if the argument is an array
    if (items.is_array())
    {
        auto array = items.as_generic_array();
        double itemsSum{};
        for (size_t idx = 0; idx < array->size(); idx++)
        {
            auto item = array->get(idx);
            itemsSum += item.convert_to<Double>().as_double();
        }

        // set the result
        m_context = wrap_DO<Double>(itemsSum).as_object();
    }
    // otherwise throw an exception
    else
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
}

void HermesASTInterpreter::toArray(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toArray(std::move(contextValue));
}


void HermesASTInterpreter::toArray(ContextValue&& ivalue)
{
    auto value = getPathObject(ivalue);

    // evaluate to the argument if it's an array
    if (value.is_array())
    {
        m_context = assignContextValue(std::move(value));
    }
    // otherwise create a single element array with the argument as the only
    // item in it
    else
    {
        auto result = make_array();
        result = result.push_back(value);

        m_context = result.as_object();
    }
}

void HermesASTInterpreter::toString(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toString(std::move(contextValue));
}


void HermesASTInterpreter::toString(ContextValue&& ivalue)
{
    auto value = getPathObject(ivalue);

    // evaluate to the argument if it's a string
    if (value.is_varchar())
    {
        m_context = assignContextValue(std::move(value));
    }
    // otherwise convert the value to a string by serializing it
    else
    {
        m_context = wrap_DO<Varchar>(value.to_plain_string()).as_object();
    }
}

void HermesASTInterpreter::toDouble(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toDouble(std::move(contextValue));
}


void HermesASTInterpreter::toDouble(ContextValue&& ivalue)
{
    auto value = getPathObject(ivalue);

    // evaluate to the argument if it's a number
    if (value.is_numeric())
    {
        m_context = assignContextValue(std::move(value));
        return;
    }
    // if it's a string
    else if (value.is_varchar())
    {
        // try to convert the string to a number
        try
        {
            m_context = wrap_DO<Double>(std::stod(value.to_str())).as_object();
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


void HermesASTInterpreter::toBigInt(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toBigInt(std::move(contextValue));
}


void HermesASTInterpreter::toBigInt(ContextValue&& ivalue)
{
    auto value = getPathObject(ivalue);

    // evaluate to the argument if it's a number
    if (value.is_bigint())
    {
        m_context = assignContextValue(std::move(value));
        return;
    }
    else
    {
        m_context = value.template convert_to<BigInt>();
    }
}


void HermesASTInterpreter::toBoolean(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    toBoolean(std::move(contextValue));
}


void HermesASTInterpreter::toBoolean(ContextValue&& ivalue)
{
    auto value = getPathObject(ivalue);

    // evaluate to the argument if it's a number
    if (value.is_bigint())
    {
        m_context = assignContextValue(std::move(value));
        return;
    }
    else if (value.is_array()) {
        auto array = value.as_generic_array();
        m_context = HermesCtr::wrap_dataobject<Boolean>(array->size() > 0).as_object();
    }
    else if (value.is_map()) {
        auto map = value.as_generic_map();
        m_context = HermesCtr::wrap_dataobject<Boolean>(map->size() > 0).as_object();
    }
    else {
        m_context = value.template convert_to<Boolean>();
    }
}

void HermesASTInterpreter::type(FunctionArgumentList &arguments)
{
    // get the first argument
    const Object& value = getArgument(arguments[0]);

    if (!value.is_null()) {
        String result = value.type_str();
        m_context = wrap_DO<Varchar>(result).as_object();
    }
    else {
        m_context = {};
    }
}

void HermesASTInterpreter::values(FunctionArgumentList &arguments)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    values(std::move(contextValue));
}


void HermesASTInterpreter::values(ContextValue&& iobject)
{
    auto object = getPathObject(iobject);

    // throw an exception if the argument is not an object
    if (!object.is_map())
    {
        BOOST_THROW_EXCEPTION(InvalidFunctionArgumentType());
    }
    // copy or move all values from object into the list of results based on
    // the type of object argument
    auto result = make_array();
    auto map = object.as_generic_map();

    map->for_each([&](auto, auto val){
        result = result.push_back(val);
    });

    m_context = result.as_object();
}

void HermesASTInterpreter::max(FunctionArgumentList &arguments,
                       const PathComparator &comparator)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    max(&comparator, std::move(contextValue));
}


void HermesASTInterpreter::max(const PathComparator* comparator, ContextValue&& iarray)
{
    auto array = getPathObject(iarray);
    hermes::Object max_val;

    auto garray = array.as_generic_array();
    for (size_t idx = 0; idx < garray->size(); idx++)
    {
        auto item = garray->get(idx);
        if (max_val.is_not_null()) {
            if (max_val.compare(item) > 0) {
                max_val = item;
            }
        }
        else {
            max_val = item;
        }
    }

    m_context = max_val;
}

void HermesASTInterpreter::maxBy(FunctionArgumentList &arguments,
                         const PathComparator &comparator)
{
    // get the first argument
    ContextValue& contextValue = getArgument<ContextValue>(arguments[0]);
    // get the second argument
    const ASTNodePtr& expression
            = getArgument<ASTNodePtr>(arguments[1]);

    maxBy(expression, &comparator, std::move(contextValue));
}


void HermesASTInterpreter::maxBy(const ASTNodePtr& expression,
                         const PathComparator* comparator,
                         ContextValue&& isource)
{
    auto source = getPathObject(isource);
    auto array  = source.as_generic_array();

    // if the array is not empty
    if (array->size())
    {
        using MaxByT = std::pair<Object, Object>;
        std::vector<MaxByT> expressionResults;
        for (size_t idx = 0; idx < array->size(); idx++)
        {
            auto item = array->get(idx);
            // evaluate the expression on the current item
            m_context = assignContextValue(item);
            visit(expression);
            Object result = getPathObject(m_context);
            expressionResults.push_back(MaxByT{item, result});
        }


        // find the largest item in the vector of results
        auto maxResultsIt = rng::max_element(expressionResults,
                                             [&](const auto& contextLeft,
                                             const auto& contextRight) {
            const Object& left = getPathObject(contextLeft.second);
            const Object& right = getPathObject(contextRight.second);
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

void HermesASTInterpreter::visit(const Object& node)
{
    if (node.is_not_null()) {
        visit(node.as_tiny_object_map());
    }
}

void HermesASTInterpreter::visit(const ASTNodePtr& node)
{
    auto attr = node.get(CODE_ATTR);
    if (attr.is_not_null())
    {
        int64_t code = attr.to_i64();
        const auto& map = visitors_map();
        auto ii = map.find(code);
        if (ii != map.end()) {
            (this->*ii->second)(node);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Unknown '{}' value: {}", CODE_ATTR, code).do_throw();
        }
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Expected '{}' attribute is null", CODE_ATTR).do_throw();
    }
}


HermesASTInterpreter::VisitorsMap HermesASTInterpreter::build_visitors_map()
{
    VisitorsMap map;

    map[NULL_NODE.code()] = &HermesASTInterpreter::visitNullNode;

    map[IDENTIFIER_NODE.code()]       = &HermesASTInterpreter::visitIdentifierNode;
    map[RAW_STRING_NODE.code()]       = &HermesASTInterpreter::visitRawStringNode;
    map[HERMES_VALUE_NODE.code()]     = &HermesASTInterpreter::visitHermesValueNode;
    map[SUBEXPRESSION_NODE.code()]    = &HermesASTInterpreter::visitSubexpressionNode;
    map[INDEX_EXPRESSION_NODE.code()] = &HermesASTInterpreter::visitIndexExpressionNode;
    map[ARRAY_ITEM_NODE.code()]       = &HermesASTInterpreter::visitArrayItemNode;
    map[FLATTEN_OPERATOR_NODE.code()] = &HermesASTInterpreter::visitFlattenOperatorNode;
    map[SLICE_EXPRESSION_NODE.code()] = &HermesASTInterpreter::visitSliceExpressionNode;
    map[LIST_WILDCARD_NODE.code()]    = &HermesASTInterpreter::visitListWildcardNode;
    map[HASH_WILDCARD_NODE.code()]    = &HermesASTInterpreter::visitHashWildcardNode;
    map[MULTISELECT_LIST_NODE.code()] = &HermesASTInterpreter::visitMultiselectListNode;
    map[MULTISELECT_HASH_NODE.code()] = &HermesASTInterpreter::visitMultiselectHashNode;
    map[NOT_EXPRESSION_NODE.code()]   = &HermesASTInterpreter::visitNotExpressionNode;
    map[COMPARATOR_EXPRESSION_NODE.code()] = &HermesASTInterpreter::visitComparatorExpressionNode;
    map[OR_EXPRESSION_NODE.code()]    = &HermesASTInterpreter::visitOrExpressionNode;
    map[AND_EXPRESSION_NODE.code()]   = &HermesASTInterpreter::visitAndExpressionNode;
    map[PAREN_EXPRESSION_NODE.code()] = &HermesASTInterpreter::visitParenExpressionNode;
    map[PIPE_EXPRESSION_NODE.code()]  = &HermesASTInterpreter::visitPipeExpressionNode;
    map[CURRENT_NODE.code()]          = &HermesASTInterpreter::visitCurrentNode;
    map[FILTER_EXPRESSION_NODE.code()] = &HermesASTInterpreter::visitFilterExpressionNode;
    map[FUNCTION_EXPRESSION_NODE.code()] = &HermesASTInterpreter::visitFunctionExpressionNode;
    map[EXPRESSION_ARGUMENT_NODE.code()] = &HermesASTInterpreter::visitExpressionArgumentNode;

    return map;
}

const HermesASTInterpreter::VisitorsMap& HermesASTInterpreter::visitors_map() {
    static VisitorsMap map = build_visitors_map();
    return map;
}

Object HermesASTInterpreter::expect_attr(const ASTNodePtr& map, const NamedCode& code)
{
    auto res = map.get(code);
    if (MMA_LIKELY(res.is_not_null())) {
        return res;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Expected '{}' attribute is null", code).do_throw();
    }
}

}} // namespace hermes::path::interpreter
