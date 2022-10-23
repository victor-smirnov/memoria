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
#include "ast/variantnode.h"
#include "ast/identifiernode.h"

#include <catch2/catch.hpp>

TEST_CASE("VariantNode")
{
    using namespace memoria::hermes::path::ast;
    using namespace fakeit;

    SECTION("is default constructible")
    {
        REQUIRE_NOTHROW(VariantNode<IdentifierNode>{});
    }

    SECTION("can be constructed with acceptable variant value")
    {
        REQUIRE_NOTHROW(VariantNode<IdentifierNode>{IdentifierNode{}});
    }

    SECTION("accepts variant value assignments")
    {
        VariantNode<IdentifierNode> variantNode;

        REQUIRE_NOTHROW(variantNode = IdentifierNode{});
    }

    SECTION("can be copy constructed")
    {
        VariantNode<IdentifierNode> node;

        REQUIRE_NOTHROW(VariantNode<IdentifierNode>(node));
    }

    SECTION("is copy assignable")
    {
        VariantNode<IdentifierNode> node1(IdentifierNode{"name"});
        VariantNode<IdentifierNode> node2;

        node2 = node1;

        bool result = node1.value == node2.value;
        REQUIRE(result);
    }

    SECTION("is comparable")
    {
        VariantNode<IdentifierNode> node1(IdentifierNode{"name"});
        VariantNode<IdentifierNode> node2;
        node2 = node1;

        REQUIRE(node1 == node2);
        REQUIRE(node1 == node1);
    }

    SECTION("is null when default constructed")
    {
        VariantNode<IdentifierNode> node;

        REQUIRE(node.isNull());
    }

    SECTION("calls visit method of visitor on accept")
    {
        using memoria::hermes::path::interpreter::AbstractVisitor;
        VariantNode<IdentifierNode> node = IdentifierNode{};
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*)))
                .AlwaysReturn();

        node.accept(&visitor.get());

        Verify(OverloadedMethod(visitor, visit, void(const IdentifierNode*)))
                .Once();
        VerifyNoOtherInvocations(visitor);
    }
}
