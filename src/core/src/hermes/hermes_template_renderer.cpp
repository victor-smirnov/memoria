
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
    using VisitorFn   = void (TplRenderer::*)(const ObjectMapPtr&);
    using VisitorsMap = ska::flat_hash_map<int64_t, VisitorFn>;

    TplVarStack stack_;
    std::ostream& out_;

public:
    TplRenderer(ObjectPtr data, std::ostream& out):
        stack_(data), out_(out)
    {}

    ObjectPtr evaluateExpr(const ObjectMapPtr& expr)
    {
        path::interpreter2::HermesASTInterpreter iterpreter;
        iterpreter.setContext(&stack_);
        iterpreter.visit(expr);

        return boost::get<ObjectPtr>(iterpreter.currentContext());
    }

    void visit(const ObjectPtr& element)
    {
        if (element->is_not_null())
        {
            if (element->is_varchar()) {
                visitText(element->as_varchar());
            }
            else if (element->is_object_array()) {
                visitStatements(element->as_object_array());
            }
            else if (element->is_object_map())
            {
                ObjectMapPtr map = element->as_object_map();
                int64_t code = map->get(CODE_ATTR)->to_i64();

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
                    "Provided Hermes Template AST node is not an ObjectMap: {}",
                    element->to_pretty_string()
                ).do_throw();
            }
        }
    }

    void visitText(const StringValuePtr& element)
    {
        auto vv = element->view();
        out_ << *vv;
    }

    void visitStatements(const ObjectArrayPtr& element)
    {
        for (uint64_t c = 0; c < element->size(); c++) {
            visit(element->get(c));
        }
    }

    void visitForStmt(const ObjectMapPtr& element)
    {
        U8StringView var_name = *element->get(VARIABLE_NAME)->as_data_object<Varchar>()->view();
        auto expr = element->get(EXPRESSION_NAME);
        auto value = evaluateExpr(expr->as_object_map());

        auto stmts = element->get(STATEMENTS_NAME);

        if (value->is_array())
        {
            auto array = value->as_generic_array();

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

    void visitIfStmt(const ObjectMapPtr& element)
    {
        auto expr = element->get(EXPRESSION_NAME);
        auto value = evaluateExpr(expr->as_object_map());

        bool boolValue = path::interpreter2::HermesASTInterpreter::toSimpleBoolean(value);
        if (boolValue)
        {
            auto stmts = element->get(STATEMENTS_NAME);
            visitStatements(stmts->as_object_array());
        }
        else {
            auto else_part = element->get(ELSE_NAME);
            visit(else_part);
        }
    }

    void visitElseStmt(const ObjectMapPtr& element)
    {
        auto stmts = element->get(STATEMENTS_NAME);
        visitStatements(stmts->as_object_array());
    }

    void visitSetStmt(const ObjectMapPtr& element)
    {
        U8StringView var_name = *element->get(VARIABLE_NAME)->as_data_object<Varchar>()->view();
        auto expr  = element->get(EXPRESSION_NAME);
        auto value = evaluateExpr(expr->as_object_map());
        stack_.set(var_name, value);
    }

    void visitVarStmt(const ObjectMapPtr& element)
    {
        ObjectMapPtr expr = element->get(EXPRESSION_NAME)->as_object_map();
        auto res = evaluateExpr(expr);
        out_ << res->to_plain_string();
    }

private:
    static VisitorsMap build_visitors_map()
    {
        VisitorsMap map;

        map[FOR_STMT_CODE]  = &TplRenderer::visitForStmt;
        map[IF_STMT_CODE]   = &TplRenderer::visitIfStmt;
        map[ELSE_STMT_CODE] = &TplRenderer::visitElseStmt;
        map[SET_STMT_CODE]  = &TplRenderer::visitSetStmt;
        map[VAR_STMT_CODE]  = &TplRenderer::visitVarStmt;

        return map;
    }

    static const VisitorsMap& visitors_map() {
        static thread_local VisitorsMap visitor_map = build_visitors_map();
        return visitor_map;
    }

};


void render(const ObjectPtr& tpl, const ObjectPtr& data, std::ostream& out)
{
    TplRenderer renderer(data, out);
    renderer.visit(tpl);
}

void render(U8StringView tpl, const ObjectPtr& data, std::ostream& out)
{
    auto parsed_tpl = parse_template(tpl, false);
    render(parsed_tpl->root(), data, out);
}

}
