// Copyright 2019 Victor Smirnov
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

//#define BOOST_SPIRIT_DEBUG

#define BOOST_SPIRIT_UNICODE

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <memoria/v1/core/sdn/sdn2.hpp>

#include <memoria/v1/core/iovector/io_buffer_base.hpp>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>


#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>

#include <boost/optional/optional_io.hpp>


#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace memoria {
namespace v1 {

namespace x3 = boost::spirit::x3;
//namespace qi    = boost::spirit::qi;
namespace ascii = x3::ascii;
namespace bp    = boost::phoenix;
//namespace bs    = boost::spirit;

}}

//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeCtrArg, value_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeArgMapEntry<memoria::v1::DataTypeCtrArg>, key_, value_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeArgMap<memoria::v1::DataTypeCtrArg>, entries_);

//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::DataTypeDeclaration, name_tokens_, parameters_, constructor_args_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::TypedStringValue, text_, type_);

//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNValue, value_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNEntry, key_, value_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::TypedSDNArray, array_, type_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::TypedSDNMap, entries_, type_);

//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNTypeDictionaryEntry, type_id_, type_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNTypeDictionary, entries_);
//BOOST_FUSION_ADAPT_STRUCT(memoria::v1::SDNDocument, type_dictionary_, value_);


namespace memoria {
namespace v1 {


//std::ostream& operator<<(std::ostream& out, const boost::recursive_wrapper<memoria::v1::DataTypeDeclaration>& decl)
//{
//    return out;
//}

class SDN2DocumentBuilder {

    SDN2Document doc_;

    ArenaBuffer<char> chars_;

public:
    static SDN2DocumentBuilder& current () {
        static thread_local SDN2DocumentBuilder builder;
        return builder;
    }

    void reset() {
        doc_.arena_.clear();
    }

    SDN2Document& doc() {return doc_;}
};



namespace {

template <typename Iterator>
bool parse_numbers(Iterator first, Iterator last, std::vector<double>& v)
{
    using x3::double_;
    using x3::phrase_parse;
    using x3::_attr;
    using ascii::space;

    auto push_back = [&](auto& ctx){ v.push_back(_attr(ctx)); };

    SDN2DocumentBuilder::current().reset();


    bool r = phrase_parse(first, last,

        //  Begin grammar
        (
            double_[push_back]
                >> *(',' >> double_[push_back])
        )
        ,
        //  End grammar

        space);

    if (first != last) // fail if we did not get a full match
        return false;
    return r;
}

}


SDN2Document SDN2Document::parse(U8StringView::const_iterator start, U8StringView::const_iterator end)
{
    std::vector<double> nums;
    bool result = parse_numbers(start, end, nums);

    std::cout << result << std::endl;

    return SDN2Document();
}


}}
