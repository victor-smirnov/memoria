
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
#include <memoria/core/tools/uid_256.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

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

using AtomT = UID256::AtomT;
using Type  = UID256::Type;

namespace {

class RNG {
    std::random_device rd_;
    std::mt19937_64 mt64_;
    std::uniform_int_distribution<AtomT> distrib_;
public:
    RNG(): mt64_(rd_()){}

    AtomT gen() {
        return distrib_(mt64_);
    }
};

RNG& get_RNG()
{
    static thread_local RNG rng;
    return rng;
}

}

UID256 UID256::make_random()
{
    UID256 uuid;

    for (size_t c = 0; c < UID256::NUM_ATOMS; c++) {
        uuid.set_atom(c, get_RNG().gen());
    }

    uuid.set_type(UID256::Type::TYPE1);

    return uuid;
}

UID256 UID256::make_type2(AtomT meta, AtomT value) {
    return make_type2(make_random(), meta, value);
}

UID256 UID256::make_type3(AtomT meta, AtomT value)
{
    return make_type3(make_random(), meta, value);
}


U8String UID256::to_u8() const
{
    std::stringstream ss;
    ss << *this;
    return U8String(ss.str());
}

U16String UID256::to_u16() const
{
    return to_u8().to_u16();
}

U8String UID256::to_cxx_decl() const
{
    std::stringstream ss;
    ss << "UID256{";

    bool first = true;
    for (size_t c = 0; c < NUM_ATOMS; c++) {
        if (!first) {
            ss << ", ";
        }
        else {
            first = false;
        }

        ss << atoms_[c] << "ull";
    }

    ss << "}";

    return ss.str();
}

AnyID UID256::as_any_id() const {
    return AnyID{std::make_unique<DefaultAnyIDImpl<UID256>>(*this)};
}

namespace {

struct UID256Content {
    uint32_t type{0};
    Optional<std::string> payload;
    Optional<AtomT> metadata;
    Optional<AtomT> counter;
};


thread_local UID256Content* parser_data{};

template <typename Iterator>
struct UID256Parser : qi::grammar<Iterator, qi::space_type>
{
    UID256Parser() : UID256Parser::base_type(uuid)
    {
        using qi::long_;
        using qi::lit;
        using qi::standard::char_;
        using qi::_1;
        using qi::_val;

        auto set_type = [](const auto& tt){
            parser_data->type = tt;
        };

        auto set_payload = [](const auto& tt){
            parser_data->payload = tt;
        };

        auto set_metadata = [](const auto& tt){
            parser_data->metadata = tt;
        };

        auto set_counter = [](const auto& tt){
            parser_data->counter = tt;
        };

        uuid %= lit("{") >> -content >> lit("}");
        content %= type [set_type] >> -('|' >> random_payload[set_payload] >> -('|' >> metadata[set_metadata] >> '|' >> counter[set_counter]));
        random_payload %= +char_("0-9a-fA-F");
        metadata %= qi::ulong_long;
        counter %= qi::ulong_long;
        type %= qi::uint_;

        BOOST_SPIRIT_DEBUG_NODE(uuid);
        BOOST_SPIRIT_DEBUG_NODE(content);
        BOOST_SPIRIT_DEBUG_NODE(random_payload);
        BOOST_SPIRIT_DEBUG_NODE(metadata);
        BOOST_SPIRIT_DEBUG_NODE(counter);
        BOOST_SPIRIT_DEBUG_NODE(type);
    }

    qi::rule<Iterator, qi::space_type> uuid;
    qi::rule<Iterator, qi::space_type> content;
    qi::rule<Iterator, uint32_t(), qi::space_type> type;
    qi::rule<Iterator, std::string(), qi::space_type> random_payload;
    qi::rule<Iterator, AtomT(), qi::space_type> metadata;
    qi::rule<Iterator, AtomT(), qi::space_type> counter;
};



}

template <typename Iterator>
UID256 UID256_parse(Iterator first, Iterator last)
{
    static thread_local UID256Parser<Iterator> const grammar;

    UID256Content content;
    parser_data = &content;

    bool r = qi::phrase_parse(first, last, grammar, qi::standard::space_type());

    if (!r || first != last) {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid UID256 string format: {}", std::string(first, last)).do_throw();
    }

    UID256 uuid;

    if (content.type > 0) {

        using Type = UID256::Type;

        auto type_val = content.type;
        Type type = static_cast<Type>(type_val);

        auto& payload = content.payload;
        auto& metadata = content.metadata;
        auto& counter = content.counter;

        if (type_val <= 3)
        {
            uuid.set_type(type);

            if (payload) {
                if (type_val > 0)
                {
                    for (size_t c = 0; c < payload->length(); c++) {
                        char buf[2] = {0, 0};
                        buf[0] = *(payload->data() + c);
                        UID256::AtomT digit = std::strtoul(buf, nullptr, 16);
                        uuid.set_payload_4bit(c, digit);
                    }
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Payload is not allowed for UID256 TYPE0").do_throw();
                }
            }

            if (metadata)
            {
                if (type_val == 2)
                {
                    uuid.set_metadata2(metadata.value());
                }
                else if (type_val == 3)
                {
                    uuid.set_metadata3(metadata.value());
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Metadata is not allowed for UID256 TYPE{}", type_val).do_throw();
                }
            }

            if (counter)
            {
                if (type_val == 2 || type_val == 3)
                {
                    uuid.set_atom(0, counter.value());
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Counter is not allowed for UID256 TYPE{}", type_val).do_throw();
                }
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Unknown UID256 type: {}", type_val).do_throw();
        }
    }

    return uuid;
}


UID256 UID256::parse(U8StringView in)
{
    return UID256_parse(in.begin(), in.end());
}

std::ostream& operator<<(std::ostream& out, const UID256& uuid)
{
    std::ios_base::fmtflags flags( out.flags() );
    out << std::dec;

    out << "{";

    using Type = UID256::Type;

    if (uuid.type() == Type::TYPE0) {

    }
    else {
        out << static_cast<UID256::AtomT>(uuid.type());
        out << "|";

        out << std::hex;

        for (size_t qq = 0; qq < uuid.payload_length(); qq++) {
            out << uuid.get_payload_4bit(qq);
        }

        out << std::dec;

        if (uuid.type() == Type::TYPE2 || uuid.type() == Type::TYPE3) {
            out << "|";
            out << uuid.metadata();
            out << "|";
            out << uuid.counter();
        }
        else if (uuid.type() == Type::TYPE1) {}
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Unknown UID256 type: {}", static_cast<UID256::AtomT>(uuid.type())).do_throw();
        }
    }

    out << "}";

    out.flags(flags);

    return out;
}

std::istream& operator>>(std::istream& in, UID256& uuid)
{
    using Iter = boost::spirit::istream_iterator;

    Iter begin(in), end;
    uuid = UID256_parse(begin, end);

    return in;
}

namespace {

UID256 debug_uid_0{};

}

UID256& get_debug_uid256_0() {
    return debug_uid_0;
}

void set_debug_uid256_0(const UID256& val) {
    debug_uid_0 = val;
}

}
