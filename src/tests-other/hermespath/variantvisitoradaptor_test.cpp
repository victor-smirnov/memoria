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
#include "ast/variantvisitoradaptor.h"
#include <memoria/core/hermes/path/exceptions.h>
#include "ast/identifiernode.h"

#include <catch2/catch.hpp>

TEST_CASE("VariantVisitorAdaptor")
{
    using namespace memoria::hermes::path;
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;

    SECTION("can be constructed with visitor")
    {
        Mock<AbstractVisitor> visitor;

        REQUIRE_NOTHROW(VariantVisitorAdaptor(&visitor.get()));
    }

    SECTION("can't be constructed with nullptr")
    {
        REQUIRE_THROWS_AS(VariantVisitorAdaptor(nullptr), InvalidAgrument);
    }

    SECTION("calls visit method of visitor with node in variant")
    {
        boost::variant<boost::blank,
                boost::recursive_wrapper<IdentifierNode> > variant;
        variant = IdentifierNode{};
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*)))
                .AlwaysReturn();

        boost::apply_visitor(VariantVisitorAdaptor(&visitor.get()), variant);

        Verify(OverloadedMethod(visitor, visit, void(const IdentifierNode*)))
                .Once();
        VerifyNoOtherInvocations(visitor);
    }

    SECTION("ignores empty variants")
    {
        boost::variant<boost::blank,
                boost::recursive_wrapper<IdentifierNode> > variant;
        Mock<AbstractVisitor> visitor;
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*)))
                .AlwaysReturn();

        boost::apply_visitor(VariantVisitorAdaptor(&visitor.get()), variant);

        VerifyNoOtherInvocations(visitor);
    }
}
