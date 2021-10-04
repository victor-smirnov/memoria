
// Copyright 2021 Victor Smirnov
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

#include <memoria/reactor/socket.hpp>
#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/tools/result.hpp>

#include <string>
#include <string.h>
#include <iostream>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;

qi::uint_parser<uint8_t, 10, 1, 3> octet;

namespace memoria {
namespace reactor {

struct V4AddressData { union { uint32_t as_uint; uint8_t as_uchar[4]; } raw; };

V4AddressData make_ipv4(uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4) {
    V4AddressData ip;
    ip.raw.as_uchar[0] = o1; ip.raw.as_uchar[1] = o2;
    ip.raw.as_uchar[2] = o3; ip.raw.as_uchar[3] = o4;
    return ip;
}

struct V4Address : qi::grammar<const char *, V4AddressData()> {
    V4Address() : V4Address::base_type(start) {
        start = ( octet >> qi::lit('.') >> octet >> qi::lit('.') >>
                  octet >> qi::lit('.') >> octet
                ) [
                    qi::_val = phx::bind(make_ipv4, qi::_1, qi::_2, qi::_3, qi::_4)
                ]
        ;
    }
    qi::rule<const char *, V4AddressData()> start;
} ipv4_address;


IPAddress parse_ipv4(U8StringView str)
{
    V4AddressData ip_data;
    bool r = qi::parse(str.begin(), str.end(), ipv4_address, ip_data);
    if (r)
    {
        return IPAddress(ip_data.raw.as_uchar, true);
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid IPv4 address format: {}", str).do_throw();
    }
}

IPAddress parse_ipv6(U8StringView str) {
    MEMORIA_MAKE_GENERIC_ERROR("Parsing IPv6 address is not yet implemented: {}", str).do_throw();
}


}}
