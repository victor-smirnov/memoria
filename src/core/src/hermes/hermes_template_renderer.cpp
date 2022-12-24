
// Copyright 2022 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/flat_map/flat_hash_map.hpp>

#include "hermes_template_parser.hpp"
#include "hermes_template_renderer.hpp"

#include "path/interpreter/hermes_ast_interpreter.h"

#include <vector>
#include <unordered_map>

namespace memoria::hermes {

class TplRenderer: public TemplateConstants {

    using ASTNodeT = TinyObjectMap;
    using HermesExprPtr = TinyObjectMap;

    using VisitorFn   = void (TplRenderer::*)(const ASTNodePtr&);
    using VisitorsMap = ska::flat_hash_map<int32_t, VisitorFn>;

    Object data_;
    TplVarStack stack_;
    std::ostream& out_;

    path::HermesObjectResolver name_resoler_;

public:
    TplRenderer(Object data, std::ostream& out):
        data_(data),
        stack_(), out_(out)
    {
        name_resoler_ = [&](U8StringView name) -> Object {
            auto prop = stack_.find(name);

            if (prop.is_initialized()) {
                return prop.get();
            }
            else {
                if (data_.is_map())
                {
                    auto map = data_.as_generic_map();
                    return map->get(name);
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Provided default data is not a MapView").do_throw();
                }
            }
        };
    }

    Object evaluateExpr(const HermesExprPtr& expr)
    {
        path::interpreter::HermesASTInterpreter iterpreter;

        iterpreter.setContext(&name_resoler_);
        iterpreter.visit(expr);
        return iterpreter.currentContext();
    }

    void visit(const Object& element)
    {
        if (element.is_not_null())
        {
            if (element.is_varchar()) {
                visitText(element.as_varchar());
            }
            else if (element.is_object_array()) {
                visitStatements(element.as_object_array());
            }
            else if (element.is_tiny_object_map())
            {
                auto map = element.as_tiny_object_map();
                int32_t code = map.get(CODE).to_i32();
                auto& vmap = visitors_map();
                auto ii = vmap.find(code);
                if (ii != vmap.end()) {
                    (this->*ii->second)(map);
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Unknown Hermes Template AST code: {}", code).do_throw();
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR(
                    "Provided Hermes Template AST node is not an TinyObjectMapView: {}",
                    element.to_pretty_string()
                ).do_throw();
            }
        }
    }

    void visitText(const DTView<Varchar>& element)
    {
        out_ << element;
    }

    void visitStatements(const ObjectArray& element)
    {
        for (uint64_t c = 0; c < element.size(); c++) {
            visit(element.get(c));
        }
    }

    void visitForStmt(const ASTNodeT& element)
    {
        auto var_name = element.get(VARIABLE).as_data_object<Varchar>();
        auto expr = element.get(EXPRESSION);
        auto value = evaluateExpr(expr.as_tiny_object_map());

        auto stmts = element.get(STATEMENTS);

        if (value.is_array())
        {
            auto array = value.as_generic_array();

            for (uint64_t c = 0; c < array->size(); c++)
            {
                auto item = array->get(c);
                stack_.start_frame(var_name, item);
                visit(stmts);
                stack_.pop();
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Provided data value is not an array").do_throw();
        }
    }

    void visitIfStmt(const ASTNodeT& element)
    {
        auto expr = element.get(EXPRESSION);
        auto value = evaluateExpr(expr.as_tiny_object_map());

        bool boolValue = path::interpreter::HermesASTInterpreter::toSimpleBoolean(value);
        if (boolValue)
        {
            auto stmts = element.get(STATEMENTS);
            visitStatements(stmts.as_object_array());
        }
        else {
            auto else_part = element.get(ELSE);
            visit(else_part);
        }
    }

    void visitElseStmt(const ASTNodeT& element)
    {
        auto stmts = element.get(STATEMENTS);
        visitStatements(stmts.as_object_array());
    }

    void visitSetStmt(const ASTNodeT& element)
    {
        auto var_name = element.get(VARIABLE).as_data_object<Varchar>();
        auto expr  = element.get(EXPRESSION);
        auto value = evaluateExpr(expr.as_tiny_object_map());
        stack_.set(var_name, value);
    }

    void visitVarStmt(const ASTNodeT& element)
    {
        auto expr = element.get(EXPRESSION).as_tiny_object_map();
        auto res = evaluateExpr(expr);
        out_ << res.to_plain_string();
    }

private:
    static VisitorsMap build_visitors_map()
    {
        VisitorsMap map;

        map[FOR_STMT.code()]  = &TplRenderer::visitForStmt;
        map[IF_STMT.code()]   = &TplRenderer::visitIfStmt;
        map[ELSE_STMT.code()] = &TplRenderer::visitElseStmt;
        map[SET_STMT.code()]  = &TplRenderer::visitSetStmt;
        map[VAR_STMT.code()]  = &TplRenderer::visitVarStmt;

        return map;
    }

    static const VisitorsMap& visitors_map() {
        static thread_local VisitorsMap visitor_map = build_visitors_map();
        return visitor_map;
    }

};


void render(const Object& tpl, const Object& data, std::ostream& out)
{
    TplRenderer renderer(data, out);
    renderer.visit(tpl);
}

void render(U8StringView tpl, const Object& data, std::ostream& out)
{
    auto parsed_tpl = parse_template(tpl, false);
    render(parsed_tpl->root(), data, out);
}

}
