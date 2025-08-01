
// Copyright 2016 Victor Smirnov
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

#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/uid_64.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/memory/ptr_cast.hpp>


#include <boost/uuid/uuid.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/fusion.hpp>
#include <boost/phoenix/stl.hpp>
#include <boost/phoenix/object.hpp>

#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_auxiliary.hpp>
#include <boost/spirit/include/qi_nonterminal.hpp>
#include <boost/spirit/include/qi_action.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

#include <string.h>
#include <chrono>
#include <memory>
#include <sstream>
#include <cstdlib>
#include <random>

namespace memoria {

namespace qi = boost::spirit::qi;
namespace enc = qi::standard;
namespace bf = boost::fusion;
namespace bp = boost::phoenix;

using AtomT = UID64::AtomT;


U8String UID64::to_u8() const
{
    std::stringstream ss;
    ss << *this;
    return U8String(ss.str());
}

U16String UID64::to_u16() const
{
    return to_u8().to_u16();
}

namespace {

struct UID64Content {
    AtomT value{0};
    Optional<AtomT> metadata;
};


thread_local UID64Content* parser_data{};

template <typename Iterator>
struct UID64Parser : qi::grammar<Iterator, qi::space_type>
{
    UID64Parser() : UID64Parser::base_type(uuid)
    {
        using qi::long_;
        using qi::lit;
        using qi::standard::char_;
        using qi::_1;
        using qi::_val;

        auto set_value = [](const auto& tt){
            parser_data->value = tt;
        };

        auto set_metadata = [](const auto& tt){
            parser_data->metadata = tt;
        };

        uuid %= lit("{") >> -content >> lit("}");
        content %= qi::ulong_long [set_value] >> -('|' >> qi::ulong_long[set_metadata]);

        BOOST_SPIRIT_DEBUG_NODE(uuid);
        BOOST_SPIRIT_DEBUG_NODE(content);
    }

    qi::rule<Iterator, qi::space_type> uuid;
    qi::rule<Iterator, qi::space_type> content;
};



}

template <typename Iterator>
UID64 UID64_parse(Iterator first, Iterator last)
{
    static thread_local UID64Parser<Iterator> const grammar;

    UID64Content content;
    parser_data = &content;

    bool r = qi::phrase_parse(first, last, grammar, qi::standard::space_type());

    if (!r || first != last) {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid UID64 string format: {}", std::string(first, last)).do_throw();
    }

    UID64 uuid;

    if (content.value > 0)
    {
        uuid.set_value(content.value);

        auto& metadata = content.metadata;

        if (metadata) {
            uuid.set_metadata(metadata.value());
        }
    }

    return uuid;
}


UID64 UID64::parse(U8StringView in)
{
    return UID64_parse(in.begin(), in.end());
}

AnyID UID64::as_any_id() const {
    return AnyID{std::make_unique<DefaultAnyIDImpl<UID64>>(*this)};
}

std::ostream& operator<<(std::ostream& out, const UID64& uuid)
{
    std::ios_base::fmtflags flags( out.flags() );
    out << std::dec;

    out << "{";
    if (uuid) {
        out << uuid.value();

        AtomT meta = uuid.metadata();
        if (meta) {
            out << "|";
            out << meta;
        }
    }
    out << "}";
    out.flags(flags);

    return out;
}

std::istream& operator>>(std::istream& in, UID64& uuid)
{
    using Iter = boost::spirit::istream_iterator;

    Iter begin(in), end;
    uuid = UID64_parse(begin, end);

    return in;
}

}
