
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

#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/memory/smart_ptrs.hpp>

#include <unicode/regex.h>

#include <memory>
#include <functional>


namespace memoria {
namespace v1 {

namespace _ {
class ICURegexMatcherImpl;
class ICURegexPatternImpl;
}

class ICUParseException: public std::exception {
    U16String pattern_;
    UParseError error_;
    U8String detail_;
public:
    ICUParseException(U16String pattern, const UErrorCode& code, const UParseError& error):
        pattern_(std::move(pattern)),
        error_(error),
        detail_(
            U16String(u"ParseError: ")
            + U16String(U8String(u_errorName(code)))
            + U16String(" : ")
            + U16String(T2T<const char16_t*>(error_.preContext))
            + U16String(T2T<const char16_t*>(error_.postContext))
        )
    {}

    virtual const char* what() const noexcept {
        return detail_.data();
    }

    static void assertOk(const U16String& pattern, const UErrorCode& code, const UParseError& parse_error) {
        if (U_FAILURE(code)) {
            throw ICUParseException(pattern, code, parse_error);
        }
    }
};



class ICURegexPattern;


class ICURegexMatcher {
public:
    using Ptr = LocalSharedPtr<_::ICURegexMatcherImpl>;
private:
    Ptr ptr_;
public:
    ICURegexMatcher() {}
    ICURegexMatcher(Ptr ptr): ptr_(std::move(ptr)) {}
    ~ICURegexMatcher() noexcept;

    bool matches();
    bool matches(int64_t start);

    bool looking_at();
    bool looking_at(int64_t start);

    bool find();
    bool find(int64_t start);

    U16String group() const;
    U16String group(int32_t group_num) const;

    int32_t groups_count() const;

    int64_t start() const;
    int64_t start(int32_t group) const;

    int64_t end() const;
    int64_t end(int32_t group) const;

    ICURegexMatcher& reset();
    ICURegexMatcher& reset(int64_t start);

    U16String input() const;

    CU16ProviderPtr input_text() const;

    ICURegexMatcher& region(int64_t start, int64_t end);
    ICURegexMatcher& region(int64_t region_start, int64_t region_end, int64_t start_index);

    int64_t region_start() const;
    int64_t region_end() const;

    bool has_transparent_bounds() const;
    ICURegexMatcher& use_transaprent_bounds(bool use_it);


    bool has_anchoring_bounds() const;
    ICURegexMatcher& use_anchoring_bounds(bool use_it);

    bool hit_end() const;
    bool require_end() const;

    ICURegexPattern pattern() const;

    int32_t get_time_limit() const;
    ICURegexMatcher& set_time_limit(int32_t time_limit_ms);

    int32_t get_stack_limit() const;
    ICURegexMatcher& set_stack_limit(int32_t stack_limit_bytes);

    ICURegexMatcher& set_trace(bool do_trace);

    void reset_preserve_region();
};


class ICUPatternMatch {
    int64_t start_;
    int64_t end_;
public:
    ICUPatternMatch(): start_(), end_() {}
    ICUPatternMatch(int64_t start, int64_t end):
        start_(start), end_(end)
    {}

    int64_t start() const {return start_;}
    int64_t end() const {return end_;}
};

using ICURangeConsumerFn = std::function<void (const ICUPatternMatch&)>;


class ICURegexPattern {
    LocalSharedPtr<_::ICURegexPatternImpl> pattern_;
public:


    ICURegexPattern() {}
    ICURegexPattern(LocalSharedPtr<_::ICURegexPatternImpl> pattern): pattern_(std::move(pattern)) {}

    ~ICURegexPattern() noexcept;

    U16String pattern() const;
    uint32_t flags() const;

    ICURegexMatcher matcher(U16String input);
    ICURegexMatcher matcher(const char16_t* input);
    ICURegexMatcher matcher(const char16_t* input, int32_t length);
    ICURegexMatcher matcher(const char* input);
    ICURegexMatcher matcher(const char* input, int32_t length);
    ICURegexMatcher matcher(const CU16ProviderPtr& input, int32_t buffer_size = 512);

    std::vector<U16String> split(const U16String& str);
    void split(const CU16ProviderPtr& text, const ICURangeConsumerFn& consumer);
    void split(const U16String& text, const ICURangeConsumerFn& consumer);


    static ICURegexPattern compile(U16String pattern, uint32_t flags = 0);
    static ICURegexPattern compile(const CU16ProviderPtr& pattern, uint32_t flags = 0, int32_t buffer_size = 512);
};





}}
