/****************************************************************************
**
** Author: Róbert Márki <gsmiko@gmail.com>
** Copyright (c) 2016 Róbert Márki
** Copyright (c) 2022 Victor Smirnov
**
** This file is originally based on the jmespath.cpp project
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

#pragma once

#include "abstractvisitor.h"
#include "memoria/core/hermes/path/types.h"
#include "../ast/expressionnode.h"
#include "../ast/functionexpressionnode.h"

#include "../../hermes_template_renderer.hpp"



#include <functional>
#include <tuple>
#include <unordered_map>
#include <boost/variant.hpp>

namespace memoria::hermes::path { namespace ast {

class BinaryExpressionNode;
}} // namespace hermes::path::ast

namespace memoria::hermes::path { namespace interpreter {

/**
 * @brief Evaluation context type.
 *
 * It can hold either a @ref Object value or a @ref const HermesObjectResolver*.
 */
using ContextValue = boost::variant<MaybeObject, const HermesObjectResolver*>;

/**
 * @brief Convert the given @a value to something assignable to a @ref
 * ContextValue variable.
 * @param[in] value A @ref Object value.
 * @return Returns the parameter without any changes as an rvalue reference.
 */
inline Object&& assignContextValue(Object&& value) {
    return std::move(value);
}

inline MaybeObject&& assignContextValue(MaybeObject&& value) {
    return std::move(value);
}
/**
 * @brief Convert the given @a value to something assignable to a
 * @ref ContextValue variable.
 * @param[in] value A @ref Object value.
 * @return Returns a @ref JsonRef which refers to the given @a value.
 */
inline Object assignContextValue(const Object& value) {
    return value;
}

inline MaybeObject assignContextValue(const MaybeObject& value) {
    return value;
}


/**
 * @brief Extract the @ref Object value held by the given @a value.
 * @param[in] contextValue A @ref ContextValue variable.
 * @return Returns a constant reference to the @ref Object value held by @a value.
 */
inline const MaybeObject& getPathObject(const ContextValue& contextValue)
{
    return boost::get<MaybeObject>(contextValue);
}

/**
 * @brief The Interpreter class evaluates the AST structure on a @ref Object
 * context.
 * @sa @ref setContext @ref currentContext
 */
class HermesASTInterpreter: public ASTCodes
{
    const IParameterResolver* parameter_resolver_{nullptr};

    using ASTNodePtr = TinyObjectMap;

    using VisitorFn   = void (HermesASTInterpreter::*)(const ASTNodePtr&);
    using VisitorsMap = ska::flat_hash_map<int64_t, VisitorFn>;

public:
    /**
     * @brief Constructs an Interpreter object.
     */
    HermesASTInterpreter();
    /**
     * @brief Sets the context of the evaluation.
     * @param[in] value Object document to be used as the context.
     */
    void setContext(const ContextValue& ctx) {
        m_context = ctx;
    }

    void set_parameter_resolver(const IParameterResolver* resolver) {
        parameter_resolver_ = resolver;
    }

    /**
     * @brief Returns the current evaluation context.
     * @return @ref Object document used as the context.
     */
    const MaybeObject &currentContext() const {
        return getPathObject(m_context);
    }
    /**
     * @brief Returns the current evaluation context which can either hold a
     * value or a const reference.
     * @return @ref ContextValue used as the context.
     */
    ContextValue &currentContextValue() {
        return m_context;
    }
    /**
     * @brief Evaluates the projection of the given @a expression on the current
     * context.
     * @param[in] expression The expression that gets projected.
     */
    virtual void evaluateProjection(const ASTNodePtr& expression);

    /**
     * @brief Evaluate the given @a node on the current context value.
     * @param[in] node Pointer to the node
     * @{
     */

    void visit(const ASTNodePtr& node);
    void visit(const MaybeObject& node);

    void visitNullNode(const ASTNodePtr& node);
    void visitIdentifierNode(const ASTNodePtr& node);
    void visitRawStringNode(const ASTNodePtr& node);
    void visitHermesValueNode(const ASTNodePtr& node);
    void visitSubexpressionNode(const ASTNodePtr& node);
    void visitIndexExpressionNode(const ASTNodePtr& node);
    void visitArrayItemNode(const ASTNodePtr& node);
    void visitFlattenOperatorNode(const ASTNodePtr& node);
    void visitBracketSpecifierNode(const ASTNodePtr& node);
    void visitSliceExpressionNode(const ASTNodePtr& node);
    void visitListWildcardNode(const ASTNodePtr& node);
    void visitHashWildcardNode(const ASTNodePtr& node);
    void visitMultiselectListNode(const ASTNodePtr& node);
    void visitMultiselectHashNode(const ASTNodePtr& node);
    void visitNotExpressionNode(const ASTNodePtr& node);
    void visitComparatorExpressionNode(const ASTNodePtr& node);
    void visitOrExpressionNode(const ASTNodePtr& node);
    void visitAndExpressionNode(const ASTNodePtr& node);
    void visitParenExpressionNode(const ASTNodePtr& node);
    void visitPipeExpressionNode(const ASTNodePtr& node);
    void visitCurrentNode(const ASTNodePtr& node);
    void visitFilterExpressionNode(const ASTNodePtr& node);
    void visitFunctionExpressionNode(const ASTNodePtr& node);
    void visitExpressionArgumentNode(const ASTNodePtr& node);
    /** @}*/

    static bool toSimpleBoolean(const Object& path);

private:
    /**
     * @brief Type of the arguments in @ref FunctionArgumentList.
     */
    using FunctionArgument
        = boost::variant<boost::blank, ContextValue, ASTNodePtr>;
    /**
     * @brief List of @ref FunctionArgument objects.
     */
    using FunctionArgumentList = std::vector<FunctionArgument>;
    /**
     * @brief Function wrapper type to which HermesPath built in function
     * implementations should conform to.
     */
    using Function = std::function<void(FunctionArgumentList&)>;
    /**
     * @brief The type of comparator functions used for comparing @ref Object
     * values.
     */
    using PathComparator = std::function<bool(const Object&, const Object&)>;
    /**
     * @brief Function argument arity validator predicate.
     */
    using ArgumentArityValidator = std::function<bool(const size_t&)>;
    /**
     * @brief Describes a built in function implementation.
     *
     * The tuple's first item is the comparator function used for comparing the
     * actual number of arguments with the expected argument count, the second
     * item marks whether the funciton needs a single @ref ContextValue or more,
     * while the third item stores the callable functions wrapper.
     */
    using FunctionDescriptor = std::tuple<ArgumentArityValidator,
                                          bool,
                                          Function>;
    /**
     * @brief List of unevaluated function arguments.
     */
    using FunctionExpressionArgumentList
        = std::vector<ast::FunctionExpressionNode::ArgumentType>;    
    /**
     * @brief Stores the evaluation context.
     */
    ContextValue m_context;
    /**
     * @brief Maps the HermesPath built in function names to their
     * implementations.
     */
    // FIXME: should be static
    std::unordered_map<String, FunctionDescriptor> m_functionMap;
    /**
     * @brief Evaluates the given @a node on the evaluation @a context.
     * @param[in] node Pointer to the node.
     * @param[in] context An const lvalue reference or an rvalue reference to
     * the evaluation context.
     * @{
     */

    void visitIdentifierNode2(const ASTNodePtr& node, ContextValue&& context);
    void visitArrayItemNode2(const ASTNodePtr& node, ContextValue&& context);
    void visitFlattenOperatorNode2(const ASTNodePtr& node, ContextValue&& context);
    void visitSliceExpressionNode2(const ASTNodePtr& node, ContextValue&& context);
    void visitHashWildcardNode2(const ASTNodePtr& node, ContextValue&& context);
    void visitFilterExpressionNode2(const ASTNodePtr& node, ContextValue&& context);

    /** @}*/
    /**
     * @brief Adjust the value of the slice endpoint to make sure it's within
     * the array's bounds and points to the correct item.
     * @param[in] length The length of the array that should be sliced.
     * @param[in] endpoint The current value of the endpoint.
     * @param[in] step The slice's step variable value.
     * @return Returns the endpoint's new value.
     */
    int64_t adjustSliceEndpoint(size_t length,
                              int64_t endpoint,
                              int64_t step) const;
    /**
     * @brief Converts the @a path value to a boolean.
     * @param[in] path The @ref Object value that needs to be converted.
     * @return Returns false if @a path is a false like value (false, 0, empty
     * list, empty object, empty string, null), otherwise returns true.
     */
    hermes::Object toBoolean(const Object& path) const;



    /**
     * @brief Evaluates the projection of the given @a expression with the
     * evaluation @a context.
     * @param[in] expression The expression that gets projected.
     * @param[in] context An const lvalue reference or an rvalue reference to
     * the evaluation context.
     */
    void evaluateProjection(const ASTNodePtr& expression,
                            ContextValue&& context);

    static bool is_hermes_object(const ContextValue& context) {
        return context.which() == 0;
    }

    /**
     * @brief Evaluates a binary logic operator to the result of the left
     * side expression if it's binary value equals to @a shortCircuitValue
     * otherwise evaluates it to the result of the result of the right side
     * expression.
     * @param[in] node Pointer to the node.
     * @param[in] shortCircuitValue Specifies what should be the boolean value
     * of the left side expression's result to do short circuit evaluation.
     */
    void evaluateLogicOperator(const ASTNodePtr& node,
                               bool shortCircuitValue);
    /**
     * @brief Evaluate the given function expression @a arguments.
     *
     * Evalutate an @ref ast::ExpressionNode on the current context to a
     * @ref Object value and evaluate @ref ast::ExpressionArgumentNode to its
     * child @ref ast::ExpressionNode.
     * @param[in] arguments List of arguments.
     * @param[in] contextValue A context used for evaluating the arguments.
     * @return List of evaluated function arguments, suitable for passing to
     * built in functions.
     */
    FunctionArgumentList evaluateArguments(
            const ObjectArray& arguments,
            const ContextValue &contextValue);
    /**
     * @brief Converts the given function @a argument to the requsted type.
     * @param[in] argument A funciton argument value.
     * @tparam T The to which the @a argument should be converted.
     * @return The value held by the funciton @a argument.
     * @throws InvalidFunctionArgumentType
     */
    template <typename T>
    T& getArgument(FunctionArgument& argument) const;
    /**
     * @brief Creates a reference to the Object value held by the @a argument.
     * @param argument A funciton argument value.
     * @return Rreference to the Object value held by the @a argument.
     * @throws InvalidFunctionArgumentType
     */
    const MaybeObject& getArgument(FunctionArgument& argument) const;
    /**
     * @brief Calculates the absolute value of the first item in the given list
     * of @a arguments. The first item must be a number @ref Object value.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void abs(FunctionArgumentList& arguments);
    /**
     * @brief Calculates the average value of the items in the first item of the
     * given @a arguments. The first item must be an @ref Object array and every
     * item in the array must be a number @ref Object value.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void avg(FunctionArgumentList& arguments);
    /**
     * @brief Checks whether the first item in the given @a arguments contains
     * the second item. The first item should be either an array or string the
     * second item can be any @ref Object type.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void contains(FunctionArgumentList& arguments);
    /**
     * @brief Rounds up the first item of the given @a arguments to the next
     * highest integer value. The first item should be a @ref Object number.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void ceil(FunctionArgumentList& arguments);
    /**
     * @brief Checks whether the first item of the given @a arguments ends with
     * the second item. The first and second item of @a arguments must be a
     * @ref Object string.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void endsWith(FunctionArgumentList& arguments);
    /**
     * @brief Rounds down the first item of the given @a arguments to the next
     * lowest integer value. The first item should be a @ref Object number.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void floor(FunctionArgumentList& arguments);
    /**
     * @brief Joins every item in the array provided as the second item of the
     * given @a arguments with the first item as a separator. The first item
     * must be a string and the second item must be an array of strings.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void join(FunctionArgumentList& arguments);
    /**
     * @brief Extracts the keys from the object provided as the first item of
     * the given @a arguments.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void keys(FunctionArgumentList& arguments);
    /**
     * @brief Returns the length of the first item in the given @a arguments.
     * The first item must be either an array a string or an object.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void length(FunctionArgumentList& arguments);
    /**
     * @brief Applies the expression provided as the first item in the given
     * @a arguments to every item in the array provided as the second item in
     * @a arguments.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void map(FunctionArgumentList& arguments);
    /**
     * @brief Applies the expression provided in @a node to every item in the
     * @a array.
     * @param[in] node Pointer to the expresson node.
     * @param[in] array Either an lvalue or rvalue reference to a @ref Object
     * array.
     * @throws InvalidFunctionArgumentType
     */
    void map(const ASTNodePtr& node, ContextValue&& array);
    /**
     * @brief Accepts zero or more objects in the given @a arguments, and
     * returns a single object with subsequent objects merged. Each subsequent
     * object’s key/value pairs are added to the preceding object.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void merge(FunctionArgumentList& arguments);
    /**
     * @brief Merges the items of the @a sourceObject into @a object.
     * @param[in,out] object The object into which the items of the
     * @a sourceObject should be added.
     * @param[in] sourceObject ither an lvalue or rvalue reference to a
     * @ref Object object.
     */
    void mergeObject(Object* object, ContextValue&& sourceObject);
    /**
     * @brief Accepts one or more items in @a arguments, and will evaluate them
     * in order until a non null argument is encounted.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void notNull(FunctionArgumentList& arguments);
    /**
     * @brief Reverses the order of the first item in @a arguments. It must
     * either be an array or a string.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void reverse(FunctionArgumentList& arguments);
    /**
     * @brief Reverses the order of the @a subject. It must either be
     * an array or a string.
     * @param[in] subject A @ref Object array or string.
     * @throws InvalidFunctionArgumentType
     */
    void reverse(ContextValue&& subject);
    /**
     * @brief Sorts the first item in the given @a arguments, which must either
     * be an array of numbers or an array of strings.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void sort(FunctionArgumentList& arguments);
    /**
     * @brief Sorts the @a array, which must either be an array of numbers or
     * an array of strings.
     * @param[in] array A @ref Object array of number or strings.
     * @throws InvalidFunctionArgumentType
     */
    void sort(ContextValue&& array);
    /**
     * @brief Sorts the first item in the given @a arguments, which must either
     * be an array of numbers or an array of strings. It uses the expression
     * provided as the second item in @a arguments as the sort key.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void sortBy(FunctionArgumentList& arguments);
    /**
     * @brief Sorts the @a array, which must either be an array of numbers or
     * an array of strings. It uses the @a expression as the sort key.
     * @param[in] expression The expression which evaluates to the key that
     * should be used for comparisons during sorting.
     * @param[in] array A @ref Object array of numbers or strings.
     * @throws InvalidFunctionArgumentType
     */
    void sortBy(const ASTNodePtr& expression, ContextValue&& array);
    /**
     * @brief Checks wheather the string provided as the first item in @a
     * arguments starts with the string provided as the second item in @a
     * arguments.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void startsWith(FunctionArgumentList& arguments);
    /**
     * @brief Calculates the sum of the numbers in the array provided as the
     * first item of @a arguments.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void sum(FunctionArgumentList& arguments);
    /**
     * @brief Converts the first item of the given @a arguments to a one element
     * array if it's not already an array.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void toArray(FunctionArgumentList& arguments);
    /**
     * @brief Converts the given @a value to a one element array if it's not
     * already an array.
     * @param[in] value A @ref Object value.
     */
    void toArray(ContextValue&& value);
    /**
     * @brief Converts the first item of the given @a arguments to the @ref Object
     * encoded value if it's not already a string.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void toString(FunctionArgumentList& arguments);
    /**
     * @brief Converts the given @a value to the @ref Object encoded value if
     * it's not already a string.
     * @param[in] value A Object value.
     */
    void toString(ContextValue&& value);
    /**
     * @brief Converts the string provided as the first item in the given
     * @a arguments to a number. If it's already a number then the original
     * value is returned, all other @ref Object types are converted to null.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void toDouble(FunctionArgumentList& arguments);
    /**
     * @brief Converts the @a value to a number if it's not already a number,
     * all other @ref Object types are converted to null.
     * @param[in] value A Object value.
     */
    void toBigInt(ContextValue&& value);


    /**
     * @brief Converts the string provided as the first item in the given
     * @a arguments to a number. If it's already a number then the original
     * value is returned, all other @ref Object types are converted to null.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void toBigInt(FunctionArgumentList& arguments);

    /**
     * @brief Converts the string provided as the first item in the given
     * @a arguments to a number. If it's already a number then the original
     * value is returned, all other @ref Object types are converted to null.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void toBoolean(FunctionArgumentList& arguments);
    /**
     * @brief Converts the @a value to a number if it's not already a number,
     * all other @ref Object types are converted to null.
     * @param[in] value A Object value.
     */
    void toBoolean(ContextValue&& value);


    /**
     * @brief Converts the @a value to a number if it's not already a number,
     * all other @ref Object types are converted to null.
     * @param[in] value A Object value.
     */
    void toDouble(ContextValue&& value);


    /**
     * @brief Returns the type of the @ref Object value provided as the first
     * item in @a arguments.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void type(FunctionArgumentList& arguments);
    /**
     * @brief Extracts the values from the object provided as the first item of
     * the given @a arguments.
     * @param[in] arguments The list of the function's arguments.
     * @throws InvalidFunctionArgumentType
     */
    void values(FunctionArgumentList& arguments);
    /**
     * @brief Extracts the values from the @a object.
     * @param[in] object A @ref Object object.
     * @throws InvalidFunctionArgumentType
     */
    void values(ContextValue&& object);
    /**
     * @brief Finds the largest item in the array provided as the first item
     * in the @a arguments, it must either be an array of numbers or an array
     * of strings.
     * @param[in] arguments The list of the function's arguments.
     * @param[in] comparator The comparator function used for comparing
     * @ref Object values. It should return true if its first argument is less
     * then its second argument.
     * @throws InvalidFunctionArgumentType
     */
    void max(FunctionArgumentList& arguments, const PathComparator& comparator);
    /**
     * @brief Finds the largest item in the @a array, it must either be an array
     * of numbers or an array of strings.
     * @param[in] comparator The comparator function used for comparing
     * @ref Object values. It should return true if its first argument is less
     * then its second argument.
     * @param[in] array A @ref Object array.
     * @throws InvalidFunctionArgumentType
     */
    void max(const PathComparator* comparator, ContextValue&& array);
    /**
     * @brief Finds the largest item in the array provided as the first item
     * in the @a arguments, which must either be an array of numbers or an array
     * of strings, using the expression provided as the second item in @a
     * arguments as a key for comparison.
     * @param[in] arguments The list of the function's arguments.
     * @param[in] comparator The comparator function used for comparing
     * @ref Object values. It should return true if its first argument is less
     * then its second argument.
     * @throws InvalidFunctionArgumentType
     */
    void maxBy(FunctionArgumentList& arguments,
               const PathComparator& comparator = std::less<Object>{});
    /**
     * @brief Finds the largest item in the @a array, which must either be an
     * array of numbers or an array of strings, using the @a  expression as a
     * key for comparison.
     * @param[in] expression The expression which evaluates to the key that
     * should be used for comparisons.
     * @param[in] comparator The comparator function used for comparing
     * @ref Object values. It should return true if its first argument is less
     * then its second argument.
     * @param[in] array A @ref Object array.
     * @throws InvalidFunctionArgumentType
     */
    void maxBy(const ASTNodePtr& expression,
               const PathComparator* comparator,
               ContextValue&& array);

private:
    // Tools
    hermes::ObjectArray wrap_array(const std::vector<hermes::Object>& array);

    static VisitorsMap build_visitors_map();
    static const VisitorsMap& visitors_map();

    static Object expect_attr(const ASTNodePtr& map, const NamedCode& name);
};
}} // namespace hermes::path::interpreter
