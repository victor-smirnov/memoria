
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

//#define BOOST_SPIRIT_QI_DEBUG
#define BOOST_SPIRIT_UNICODE

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/strings/u32_string.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <memoria/core/hermes/path/types.h>

#include "hermes_internal.hpp"
#include "path/parser/grammar.h"

#include "path/parser/appendutf8action.h"
#include "path/ast/rawstringnode.h"

#include "path/ast/hermesvaluenode.h"

#include "hermes_template_parser.hpp"
#include "hermes_parser_tools.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>


namespace memoria::hermes {


using IteratorType = path::U8UnicodeIteratorAdaptor;

namespace qi  = boost::spirit::qi;
namespace enc = qi::unicode;
namespace bf  = boost::fusion;
namespace bp  = boost::phoenix;

template <typename Iterator, typename Skipper = enc::space_type>
struct HermesTemplateParser :
    path::parser::GrammarRuleSet <
        Iterator, Skipper
    >,
    qi::grammar<
        Iterator,
        path::ast::HermesValueNode(),
        Skipper
    >
{
    using RuleLib = path::parser::GrammarRuleSet <
        Iterator, Skipper
    >;

    using HermesObject = path::ast::HermesValueNode;
    using HermesArray  = path::ast::HermesArrayNode;

    using RuleLib::m_topLevelExpressionHORule;
    using RuleLib::m_identifierRule;

    HermesTemplateParser() :
        HermesTemplateParser::base_type(
            hermes_template
        )
    {
        using qi::long_;
        using qi::lit;
        using qi::lexeme;
        using qi::no_skip;
        using enc::char_;
        using qi::_val;
        using qi::_a;
        using qi::eol;
        using qi::eoi;
        using qi::_pass;
        using qi::fail;
        using qi::_1;

        using bp::construct;
        using bp::val;

        bp::function<path::parser::AppendUtf8Action<>> appendUtf8;
        text_block = no_skip[+((char_ - "{{" - "{%" - "{#")[appendUtf8(bp::at_c<0>(_val), _1)])];

        open_stmt  = lit("{%+") [_val = true] | lit("{%-")[_val = false] | lit("{%");
        close_stmt = lit("+%}") [_val = true] | lit("-%}")[_val = false] | lit("%}");

        auto text_to_hermes = [](auto& attrib, auto& ctx) {
            auto obj = current_ctr()->new_dataobject<Varchar>(attrib.rawString).as_object();
            ctx.attributes = HermesObject(obj);
        };
        text_object = text_block[text_to_hermes];

        var_stmt = lit("{{") > m_topLevelExpressionHORule > "}}";

        for_stmt = (open_stmt >> "for" >> m_identifierRule >> "in" >> m_topLevelExpressionHORule >> close_stmt)
                    >> hermes_block_sequence
                    >> endfor_stmt;


        endfor_stmt = open_stmt >> "endfor" >> close_stmt;
        endif_stmt  = open_stmt >> "endif" >> close_stmt;
        else_stmt   = open_stmt >> "else" >> close_stmt >> hermes_block_sequence >> endif_stmt;

        if_stmt = (open_stmt >> "if" >> m_topLevelExpressionHORule >> close_stmt)
                    >> hermes_block_sequence
                    >> (
                        endif_stmt |
                        else_stmt  |
                        elif_stmt
                   );

        elif_stmt = (open_stmt >> "elif" >> m_topLevelExpressionHORule >> close_stmt)
                    >> hermes_block_sequence
                    >> (
                        endif_stmt |
                        else_stmt  |
                        elif_stmt
                    );

        set_stmt = (open_stmt >> "set" >> m_identifierRule >> "=" >> m_topLevelExpressionHORule >> close_stmt);


        auto to_hermes_object = [](auto& attrib, auto& ctx){
            ctx.attributes = HermesObject(attrib);
        };

        for_stmt_object = for_stmt[to_hermes_object];
        if_stmt_object  = if_stmt[to_hermes_object];
        set_stmt_object = set_stmt[to_hermes_object];
        var_stmt_object = var_stmt[to_hermes_object];

        block_sequence = *(
                    text_object     |
                    var_stmt_object |
                    for_stmt_object |
                    if_stmt_object  |
                    set_stmt_object
                    );

        auto std_array_to_object = [](auto& attrib, auto& ctx){
            auto array = current_ctr()->make_object_array(attrib.size());
            for (auto& item: attrib) {
                array = array.push_back(std::move(item.value));
            }

            TemplateConstants::process_outer_space(array.as_object());

            ctx.attributes = HermesObject(array.as_object());
        };
        hermes_block_sequence = block_sequence[std_array_to_object];

        hermes_template = hermes_block_sequence | eoi;
    }

    qi::rule<Iterator, path::ast::RawStringNode(), Skipper> text_block;
    qi::rule<Iterator, HermesObject(), Skipper>             text_object;

    qi::rule<Iterator, TplVarStatement(), Skipper>          var_stmt;
    qi::rule<Iterator, HermesObject(), Skipper>             var_stmt_object;

    qi::rule<Iterator, TplForStatement(), Skipper>  for_stmt;
    qi::rule<Iterator, HermesObject(), Skipper>     for_stmt_object;

    qi::rule<Iterator, TplIfStatement(), Skipper>   if_stmt;
    qi::rule<Iterator, TplIfStatement(), Skipper>   elif_stmt;
    qi::rule<Iterator, HermesObject(), Skipper>     if_stmt_object;

    qi::rule<Iterator, TplSetStatement(), Skipper>  set_stmt;
    qi::rule<Iterator, HermesObject(), Skipper>     set_stmt_object;


    qi::rule<Iterator, Optional<bool>(), Skipper> open_stmt;
    qi::rule<Iterator, Optional<bool>(), Skipper> close_stmt;

    qi::rule<Iterator, TplSpaceData(), Skipper>     endif_stmt;
    qi::rule<Iterator, TplElseStatement(), Skipper> else_stmt;
    qi::rule<Iterator, TplSpaceData(), Skipper>     endfor_stmt;

    qi::rule<Iterator, std::vector<HermesObject>(), Skipper> block_sequence;
    qi::rule<Iterator, HermesObject(), Skipper> hermes_block_sequence;

    qi::rule<Iterator, HermesObject(), Skipper> hermes_template;
};


template <typename Iterator>
void parse_hermes_template(Iterator& first, Iterator& last, HermesCtr& doc, bool node_names)
{
    HermesCtrBuilderCleanup cleanup;
    HermesCtrBuilder::enter(doc.self());

    using Grammar = HermesTemplateParser<Iterator>;

    static thread_local Grammar const grammar;

    path::ast::HermesValueNode result;

    Iterator start = first;
    try
    {
        bool r = qi::phrase_parse(first, last, grammar, typename Grammar::SkipperT(), result);
        if (!r) {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes document parse failure").do_throw();
        }
        else if (first != last) {
            ErrorMessageResolver::instance().do_throw(start, first, last);
        }

        doc.set_root(result.value);
    }
    catch (const ExpectationException<Iterator>& ex) {
        ErrorMessageResolver::instance().do_throw(start, ex);
    }
}


PoolSharedPtr<HermesCtr> parse_template(U8StringView text, bool node_names) {
    PoolSharedPtr<HermesCtrImpl> doc = TL_get_reusable_shared_instance<HermesCtrImpl>();

    IteratorType beginIt(text.begin());
    IteratorType it = beginIt;
    IteratorType endIt(text.end());

    parse_hermes_template(it, endIt, *doc, node_names);

    return doc;
}

}
