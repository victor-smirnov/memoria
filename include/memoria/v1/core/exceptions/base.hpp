
// Copyright 2018 Victor Smirnov
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


#pragma once

#include <memoria/v1/core/types.hpp>

#include <boost/exception/all.hpp>

namespace memoria {
namespace v1 {

class MemoriaThrowableLW {
protected:
    const char* source_;
public:
    MemoriaThrowableLW(const char* source): source_(source) {}

    const char* source() const noexcept {
        return source_;
    }

    virtual void dump(std::ostream& out) const noexcept {}
};


struct MemoriaThrowable: virtual std::exception, virtual boost::exception {

    MemoriaThrowable() noexcept {
        boost::enable_error_info(*this);
    }

    virtual ~MemoriaThrowable() noexcept {}

    virtual void dump(std::ostream& out) const noexcept;

    virtual const char* what() const noexcept;
    virtual std::string message() const noexcept;
};


using Traced = boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>;


#define MMA1_THROW(Ex) throw Ex << ::memoria::v1::Traced(boost::stacktrace::stacktrace()) \
    << ::boost::throw_function(BOOST_THROW_EXCEPTION_CURRENT_FUNCTION) \
    << ::boost::throw_file(__FILE__) \
    << ::boost::throw_line(static_cast<int>(__LINE__))


std::ostream& operator<<(std::ostream& out, const MemoriaThrowable& t);


}}
