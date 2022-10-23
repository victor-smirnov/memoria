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
#include "interpreter/contextvaluevisitoradaptor.h"
#include "interpreter/interpreter.h"
#include <memoria/core/hermes/path/path.h>
#include "ast/identifiernode.h"
#include <boost/variant.hpp>
#include <boost/hana.hpp>
#include <functional>

#include <catch2/catch.hpp>

template <typename VisitorT, bool ForceMove = false>
using AdaptorType
    = memoria::hermes::path::interpreter::ContextValueVisitorAdaptor<VisitorT, ForceMove>;

TEST_CASE("ContextValueVisitorAdaptor")
{
    using namespace memoria::hermes::path;
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;

    SECTION("can be constructed with visitor")
    {
        auto visitor = boost::hana::overload(
            [](const Json&){},
            [](Json&){}
        );

        REQUIRE_NOTHROW(AdaptorType<decltype(visitor)>(std::move(visitor)));
    }

    SECTION("calls visitor with rvalue ref of Json value in ContextValue")
    {
        bool jsonFunctionCalled = false;
        auto visitor = boost::hana::overload(
            [](const Json&){},
            [&](Json&&){
                jsonFunctionCalled = true;
            }
        );
        AdaptorType<decltype(visitor)> adaptor(std::move(visitor));
        ContextValue contextValue {Json{}};
        REQUIRE(contextValue.which() == 0);

        boost::apply_visitor(adaptor, contextValue);

        REQUIRE(jsonFunctionCalled);
    }

    SECTION("calls visitor with lvalue ref of Json reference in ContextValue")
    {
        bool jsonRefFunctionCalled = false;
        auto visitor = boost::hana::overload(
            [&](const Json&){
                jsonRefFunctionCalled = true;
            },
            [](Json&&){}
        );
        AdaptorType<decltype(visitor)> adaptor(std::move(visitor));
        Json value;
        ContextValue contextValue{std::cref(value)};
        REQUIRE(contextValue.which() == 1);

       boost::apply_visitor(adaptor, contextValue);

       REQUIRE(jsonRefFunctionCalled);
    }

    SECTION("calls visitor with rvalue ref of the copy of value held in "
            "ContextValue")
    {
        bool jsonFunctionCalled = false;
        auto visitor = boost::hana::overload(
            [](const Json&){},
            [&](Json&&){
                jsonFunctionCalled = true;
            }
        );
        AdaptorType<decltype(visitor), true> adaptor(std::move(visitor));
        Json value;
        ContextValue contextValue{std::cref(value)};
        REQUIRE(contextValue.which() == 1);

       boost::apply_visitor(adaptor, contextValue);

       REQUIRE(jsonFunctionCalled);
    }
}

namespace {
class VisitorStub
{
public:
    virtual ~VisitorStub() {}
    virtual void visit(const memoria::hermes::path::ast::IdentifierNode *node,
                       const memoria::hermes::path::Json& context) {}
    virtual void visit(const memoria::hermes::path::ast::IdentifierNode *node,
                       memoria::hermes::path::Json&& context) {}
};
}

TEST_CASE("makeVisitor")
{
    using namespace memoria::hermes::path;
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;

    SECTION("creates ContextValue visitor from member functions and call "
            "function taking lvalue ref if called with Json reference")
    {
        using std::placeholders::_1;
        using LvalueType = void(VisitorStub::*)(const IdentifierNode*,
                                                const Json&);
        using RvalueType = void(VisitorStub::*)(const IdentifierNode*, Json&&);
        IdentifierNode node{"identifier"};
        Mock<VisitorStub> visitor;
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   const Json&)))
                .AlwaysReturn();
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   Json&&)))
                .AlwaysReturn();
        const IdentifierNode* nodePtr = &node;
        Json value;
        ContextValue contextValue{std::cref(value)};

        auto adaptor = makeVisitor(
            std::bind(static_cast<LvalueType>(&VisitorStub::visit),
                      &visitor.get(), nodePtr, _1),
            std::bind(static_cast<RvalueType>(&VisitorStub::visit),
                      &visitor.get(), nodePtr, _1)
        );
        boost::apply_visitor(adaptor, contextValue);

        Verify(OverloadedMethod(visitor, visit,
                                void(const IdentifierNode*, const Json&))
               .Using(&node, value))
                .Once();
        VerifyNoOtherInvocations(visitor);
    }

    SECTION("creates ContextValue visitor from member functions and call "
            "function taking rvalue ref if called with Json value")
    {
        using std::placeholders::_1;
        using LvalueType = void(VisitorStub::*)(const IdentifierNode*,
                                                const Json&);
        using RvalueType = void(VisitorStub::*)(const IdentifierNode*, Json&&);
        IdentifierNode node{"identifier"};
        Mock<VisitorStub> visitor;
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   const Json&)))
                .AlwaysReturn();
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   Json&&)))
                .AlwaysReturn();
        const IdentifierNode* nodePtr = &node;
        Json value;
        ContextValue contextValue{std::move(value)};

        auto adaptor = makeVisitor(
            std::bind(static_cast<LvalueType>(&VisitorStub::visit),
                      &visitor.get(), nodePtr, _1),
            std::bind(static_cast<RvalueType>(&VisitorStub::visit),
                      &visitor.get(), nodePtr, _1)
        );
        boost::apply_visitor(adaptor, contextValue);

        Verify(OverloadedMethod(visitor, visit,
                                void(const IdentifierNode*, Json&&))
               .Using(&node, std::move(boost::get<Json>(contextValue))))
                .Once();
        VerifyNoOtherInvocations(visitor);
    }
}

TEST_CASE("makeMoveOnlyVisitor")
{
    using namespace memoria::hermes::path;
    using namespace memoria::hermes::path::ast;
    using namespace memoria::hermes::path::interpreter;
    using namespace fakeit;

    SECTION("creates ContextValue visitor from member functions and call "
            "function taking rvalue ref if called with Json reference")
    {
        using std::placeholders::_1;
        using RvalueType = void(VisitorStub::*)(const IdentifierNode*, Json&&);
        IdentifierNode node{"identifier"};
        Mock<VisitorStub> visitor;
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   const Json&)))
                .AlwaysReturn();
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   Json&&)))
                .AlwaysReturn();
        const IdentifierNode* nodePtr = &node;
        Json value{15};
        ContextValue contextValue{std::cref(value)};

        auto adaptor = makeMoveOnlyVisitor(
            std::bind(static_cast<RvalueType>(&VisitorStub::visit),
                      &visitor.get(), nodePtr, _1)
        );
        boost::apply_visitor(adaptor, contextValue);

        Verify(OverloadedMethod(visitor, visit,
                                void(const IdentifierNode*, Json&&)))
                .Once();
        VerifyNoOtherInvocations(visitor);
    }

    SECTION("creates ContextValue visitor from member functions and call "
            "function taking rvalue ref if called with Json value")
    {
        using std::placeholders::_1;
        using RvalueType = void(VisitorStub::*)(const IdentifierNode*, Json&&);
        IdentifierNode node{"identifier"};
        Mock<VisitorStub> visitor;
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   const Json&)))
                .AlwaysReturn();
        When(OverloadedMethod(visitor, visit, void(const IdentifierNode*,
                                                   Json&&)))
                .AlwaysReturn();
        const IdentifierNode* nodePtr = &node;
        Json value;
        ContextValue contextValue{std::move(value)};

        auto adaptor = makeMoveOnlyVisitor(
            std::bind(static_cast<RvalueType>(&VisitorStub::visit),
                      &visitor.get(), nodePtr, _1)
        );
        boost::apply_visitor(adaptor, contextValue);

        Verify(OverloadedMethod(visitor, visit,
                                void(const IdentifierNode*, Json&&)))
                .Once();
        VerifyNoOtherInvocations(visitor);
    }
}
