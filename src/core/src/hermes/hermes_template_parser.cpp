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

#include <memoria/core/hermes/path/expression.h>

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
        ObjectPtr(),
        Skipper
    >
{
    using RuleLib = path::parser::GrammarRuleSet <
        Iterator
    >;

    using RuleLib::m_topLevelExpressionHORule;

    HermesTemplateParser() :
        HermesTemplateParser::base_type(
            hermes_template
        )
    {
        using qi::long_;
        using qi::lit;
        using qi::lexeme;
        using enc::char_;
        using qi::_val;
        using qi::eol;
        using qi::eoi;
        using qi::_pass;
        using qi::fail;
        using qi::_1;

        using bp::construct;
        using bp::val;

        hermes_template = m_topLevelExpressionHORule;
    }

    qi::rule<Iterator, ObjectPtr(), Skipper> hermes_template;
};


template <typename Iterator>
void parse_hermes_template(Iterator& first, Iterator& last, HermesCtr& doc, bool node_names)
{
    HermesCtrBuilderCleanup cleanup;
    HermesCtrBuilder::enter(doc.self());

    using Grammar = HermesTemplateParser<Iterator>;
    //using Grammar = path::parser::HOGrammar<Iterator>;

    static thread_local Grammar const grammar;

    ObjectPtr result;

    //Iterator start = first;
    //try
    {
        bool r = qi::phrase_parse(first, last, grammar, typename Grammar::SkipperT(), result);
        if (!r) {
            MEMORIA_MAKE_GENERIC_ERROR("Hermes document parse failure1").do_throw();
        }
        else if (first != last) {
            //ErrorMessageResolver::instance().do_throw(start, first, last);
            MEMORIA_MAKE_GENERIC_ERROR("Hermes document parse failure2").do_throw();
        }

        doc.set_root(result);
    }
//    catch (const ExpectationException<Iterator>& ex) {
//        ErrorMessageResolver::instance().do_throw(start, ex);
//    }
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
