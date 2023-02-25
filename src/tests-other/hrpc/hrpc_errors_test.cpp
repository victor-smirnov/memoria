
// Copyright 2023 Victor Smirnov
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

#include "hrpc_tests_common.hpp"

#include <memoria/core/exceptions/exceptions.hpp>
#include <exception>

namespace memoria {

using namespace hrpc;

constexpr U8StringView MEMORIA_ERROR     = "MEMORIA_ERROR";
constexpr U8StringView MEMORIA_EXCEPTION = "MEMORIA_EXCEPTION";
constexpr U8StringView CXX_SYSTEM_ERROR  = "CXX_SYSTEM_ERROR";
constexpr U8StringView CXX_EXCEPTION     = "CXX_EXCEPTION";
constexpr U8StringView BOOST_EXCEPTION   = "BOOST_EXCEPTION";
constexpr U8StringView OTHER_EXCEPTION   = "OTHER_EXCEPTION";


struct some_boost_exception : public boost::exception, std::exception {};

Response errors_handler(ContextPtr context)
{
    U8StringOView arg = context->request().parameters().get(ARG1).as_varchar();

    if (arg == MEMORIA_ERROR) {
        MEMORIA_MAKE_GENERIC_ERROR("Some error").do_throw();
    }
    else if (arg == MEMORIA_EXCEPTION) {
        MMA_THROW(Exception() << WhatCInfo("Error description"));
    }
    else if (arg == CXX_SYSTEM_ERROR) {
        throw std::system_error(
            std::make_error_code(std::errc::io_error)
        );
    }
    else if (arg == CXX_EXCEPTION) {
        throw std::runtime_error("Runtime Exception");
    }
    else if (arg == BOOST_EXCEPTION) {
        throw some_boost_exception() << WhatCInfo("Some Boost Exception");
    }
    else {
        throw new int{1};
    }
}

}

using namespace memoria;
using namespace memoria::hrpc;

TEST_CASE( "MemoriaError" ) {
    auto rq = Request::make();

    rq.set_parameter(ARG1, MEMORIA_ERROR );

    auto call = session()->call(ERRORS_TEST, rq);
    call->wait();

    Response rs = call->response();

    REQUIRE(rs.status_code() == StatusCode::ERROR);

    hrpc::Error error = rs.error();
    CHECK(error.type() == ErrorType::MEMORIA);
    CHECK(error.description() == "Some error");
}


TEST_CASE( "MemoriaThrowable" ) {
    auto rq = Request::make();

    rq.set_parameter(ARG1, MEMORIA_EXCEPTION );

    auto call = session()->call(ERRORS_TEST, rq);
    call->wait();

    Response rs = call->response();

    REQUIRE(rs.status_code() == StatusCode::ERROR);

    Error error = rs.error();
    CHECK(error.type() == ErrorType::MEMORIA);
    CHECK(error.description() == "Error description");
}


TEST_CASE( "C++ SystemError" ) {
    auto rq = Request::make();

    rq.set_parameter(ARG1, CXX_SYSTEM_ERROR );

    auto call = session()->call(ERRORS_TEST, rq);
    call->wait();

    Response rs = call->response();
    REQUIRE(rs.status_code() == StatusCode::ERROR);

    REQUIRE(rs.error().type() == ErrorType::CXX_SYSTEM);
    CxxSystemError error(rs.error().object());

    std::system_error serr(
        std::make_error_code(std::errc::io_error)
    );

    CHECK(error.description() == serr.what());
    CHECK(error.code() == serr.code().value());
    CHECK(error.message() == serr.code().message());
    CHECK(error.category() == serr.code().category().name());
    CHECK(error.condition_code() == serr.code().default_error_condition().value());
    CHECK(error.condition_message() == serr.code().default_error_condition().message());
    CHECK(error.condition_category() == serr.code().default_error_condition().category().name());
}


TEST_CASE( "Boost Exception" ) {
    auto rq = Request::make();

    rq.set_parameter(ARG1, BOOST_EXCEPTION );

    auto call = session()->call(ERRORS_TEST, rq);
    call->wait();

    Response rs = call->response();

    REQUIRE(rs.status_code() == StatusCode::ERROR);

    Error error = rs.error();
    REQUIRE(error.type() == ErrorType::BOOST);
    CHECK(error.description().size() > 0);
}


TEST_CASE( "C++ std::exception" ) {
    auto rq = Request::make();

    rq.set_parameter(ARG1, CXX_EXCEPTION );

    auto call = session()->call(ERRORS_TEST, rq);
    call->wait();

    Response rs = call->response();

    REQUIRE(rs.status_code() == StatusCode::ERROR);

    Error error = rs.error();
    CHECK(error.type() == ErrorType::CXX_STD);
    CHECK(error.description() == "Runtime Exception");
}


TEST_CASE( "Unknown std::exception" ) {
    auto rq = Request::make();

    rq.set_parameter(ARG1, OTHER_EXCEPTION );

    auto call = session()->call(ERRORS_TEST, rq);
    call->wait();

    Response rs = call->response();
    REQUIRE(rs.status_code() == StatusCode::ERROR);

    Error error = rs.error();
    CHECK(error.type() == ErrorType::UNKNOWN);
}

TEST_CASE( "Missing Endpoint" ) {
    auto rq = Request::make();

    auto call = session()->call(MISSING_ENDPOINT, rq);
    call->wait();

    hrpc::Response rs = call->response();
    REQUIRE(rs.status_code() == hrpc::StatusCode::ERROR);

    REQUIRE(rs.error().type() == ErrorType::HRPC);
    HrpcError error(rs.error().object());

    CHECK(error.code() == HrpcErrors::INVALID_ENDPOINT);    
    CHECK(error.endpoint_id() == MISSING_ENDPOINT);
}
