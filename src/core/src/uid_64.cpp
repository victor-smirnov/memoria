
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

#include <memoria/core/datatypes/datum.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

#include <boost/uuid/uuid.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

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

template <>
Datum<UID64> datum_from_sdn_value(const UID64*, int64_t value)
{
    MMA_THROW(RuntimeException()) << WhatCInfo("SDN convertion from int64_t to UID64 is not supported");
}

template <>
Datum<UID64> datum_from_sdn_value(const UID64*, double value) {
    MMA_THROW(RuntimeException()) << WhatCInfo("SDN convertion from double to UID64 is not supported");
}


template <>
Datum<UID64> datum_from_sdn_value(const UID64*, const U8StringView& str)
{
    UID64 uuid = UID64::parse(str);
    return Datum<UID64>(uuid);
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

        if (metadata.is_initialized()) {
            uuid.set_metadata(metadata.get());
        }
    }

    return uuid;
}


UID64 UID64::parse(U8StringView in)
{
    return UID64_parse(in.begin(), in.end());
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
