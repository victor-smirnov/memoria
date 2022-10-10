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
#include "memoria/core/hermes/path/expression.h"
#include "parser/parser.h"
#include "parser/grammar.h"

namespace memoria::jmespath {

Expression::Expression()
    : m_astRoot(new ast::ExpressionNode)
{
}

Expression::Expression(const Expression &other)
    : Expression()
{
    *this = other;
}

Expression::Expression(Expression &&other)
{
    *this = std::move(other);
}

Expression& Expression::operator=(const Expression &other)
{
    if (this != &other)
    {
        m_expressionString = other.m_expressionString;
        *m_astRoot = *other.m_astRoot;
    }
    return *this;
}

Expression& Expression::operator=(Expression &&other)
{
    if (this != &other)
    {
        m_expressionString = std::move(other.m_expressionString);
        m_astRoot = std::move(other.m_astRoot);
    }
    return *this;
}

String Expression::toString() const
{
    return m_expressionString;
}

bool Expression::isEmpty() const
{
    return (!m_astRoot || m_astRoot->isNull());
}

const ast::ExpressionNode *Expression::astRoot() const
{
    return m_astRoot.get();
}

void Expression::parseExpression(const String& expressionString)
{
    if (!m_astRoot)
    {
        m_astRoot.reset(new ast::ExpressionNode);
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
     thread_local parser::Parser<parser::Grammar> s_parser;
#pragma clang diagnostic pop
    *m_astRoot = s_parser.parse(expressionString);
}

bool Expression::operator==(const Expression &other) const
{
    if (this != &other)
    {
        return (m_expressionString == other.m_expressionString)
                && (*m_astRoot == *other.m_astRoot);
    }
    return true;
}

void Expression::ExpressionDeleter::operator()(ast::ExpressionNode *node) const
{
    if (node)
    {
        delete node;
    }
}
} // namespace jmespath
