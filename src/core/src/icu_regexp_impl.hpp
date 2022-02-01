
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

#include <memoria/core/regexp/icu_regexp.hpp>

#ifndef MMA_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif

namespace memoria {

using RegexMatcher = MMA_ICU_CXX_NS::RegexMatcher;
using RegexPattern = MMA_ICU_CXX_NS::RegexPattern;

namespace detail {

class ICURegexMatcherImpl {

    LocalSharedPtr<ICURegexPatternImpl> pattern_;
    std::unique_ptr<RegexMatcher> matcher_;

public:
    ICURegexMatcherImpl(const LocalSharedPtr<ICURegexPatternImpl>& pattern, std::unique_ptr<RegexMatcher>&& matcher):
        pattern_(pattern), matcher_(std::move(matcher))
    {}

public:
    ~ICURegexMatcherImpl() noexcept {}

    bool matches() {
        return with_icu_error([this](UErrorCode* status){
            return matcher_->matches(*status);
        });
    }

    bool matches(int64_t start) {
        return with_icu_error([&](UErrorCode* status){
            return matcher_->matches(start, *status);
        });
    }

    bool looking_at() {
        return with_icu_error([this](UErrorCode* status){
            return matcher_->lookingAt(*status);
        });
    }

    bool looking_at(int64_t start) {
        return with_icu_error([&](UErrorCode* status){
            return matcher_->lookingAt(start, *status);
        });
    }

    bool find() {
        return matcher_->find();
    }

    bool find(int64_t start) {
        return with_icu_error([&](UErrorCode* status){
            return matcher_->find(start, *status);
        });
    }

    U16String group() const
    {
        auto ptr = std::make_unique<UText>();
        *ptr = UTEXT_INITIALIZER;
        int64_t group_len{};

        with_icu_error([&](UErrorCode* status){
            matcher_->group(ptr.get(), group_len, *status);
        });

        if (group_len > 0)
        {
            int64_t start = get_native_index_for(ptr.get());

            UTextUniquePtr closeable_ptr(ptr.release(), [](UText* ut){
                utext_close(ut);
                delete ut;
            });

            return to_u16string(closeable_ptr.get(), start, start + group_len);
        }
        else {
            return U16String();
        }
    }

    U16String group(int32_t group_num) const
    {
        auto ptr = std::make_unique<UText>();
        *ptr = UTEXT_INITIALIZER;
        int64_t group_len{};

        with_icu_error([&](UErrorCode* status){
            matcher_->group(group_num, ptr.get(), group_len, *status);
        });

        if (group_len > 0)
        {
            int64_t start = get_native_index_for(ptr.get());

            UTextUniquePtr closeable_ptr(ptr.release(), [](UText* ut){
                utext_close(ut);
                delete ut;
            });

            return to_u16string(closeable_ptr.get(), start, start + group_len);
        }
        else {
            return U16String();
        }
    }

    int32_t groups_count() const {
        return matcher_->groupCount();
    }

    int64_t start() const {
        return with_icu_error([&](UErrorCode* status){
            return matcher_->start64(*status);
        });
    }

    int64_t start(int32_t group) const {
        return with_icu_error([&](UErrorCode* status){
            return matcher_->start64(group, *status);
        });
    }

    int64_t end() const {
        return with_icu_error([&](UErrorCode* status){
            return matcher_->end64(*status);
        });
    }

    int64_t end(int32_t group) const
    {
        return with_icu_error([&](UErrorCode* status){
            return matcher_->end64(group, *status);
        });
    }

    void reset() {
        return with_icu_error([&](UErrorCode* status){
            matcher_->reset(*status);
        });
    }

    void reset(int64_t start) {
        return with_icu_error([&](UErrorCode* status){
            matcher_->reset(start, *status);
        });
    }


    U16String input() const
    {
        auto ptr = std::make_unique<UText>();
        *ptr = UTEXT_INITIALIZER;

        with_icu_error([&](UErrorCode* status){
            matcher_->getInput(ptr.get(), *status);
        });


        UTextUniquePtr closeable_ptr(ptr.release(), [](UText* ut){
            utext_close(ut);
            delete ut;
        });

        return to_u16string(closeable_ptr.get());
    }

    CU16ProviderPtr input_text() const
    {
        auto ptr = std::make_unique<UText>();
        *ptr = UTEXT_INITIALIZER;

        with_icu_error([&](UErrorCode* status){
            matcher_->getInput(ptr.get(), *status);
        });


        UTextUniquePtr closeable_ptr(ptr.release(), [](UText* ut){
            utext_close(ut);
            delete ut;
        });

        return as_cu16_provider(std::move(closeable_ptr));
    }

    void region(int64_t start, int64_t end)
    {
        with_icu_error([&](UErrorCode* status){
            matcher_->region(start, end, *status);
        });
    }

    void region(int64_t region_start, int64_t region_end, int64_t start_index)
    {
        with_icu_error([&](UErrorCode* status){
            matcher_->region(region_start, region_end, start_index, *status);
        });
    }

    int64_t region_start() const {
        return matcher_->regionStart64();
    }

    int64_t region_end() const {
        return matcher_->regionEnd64();
    }

    bool has_transparent_bounds() const {
        return matcher_->hasTransparentBounds();
    }

    void use_transaprent_bounds(bool use_it) {
        matcher_->useTransparentBounds(use_it);
    }


    bool has_anchoring_bounds() const {
        return matcher_->hasAnchoringBounds();
    }

    void use_anchoring_bounds(bool use_it) {
        matcher_->useAnchoringBounds(use_it);
    }

    bool hit_end() const {
        return matcher_->hitEnd();
    }

    bool require_end() const {
        return matcher_->requireEnd();
    }

    LocalSharedPtr<ICURegexPatternImpl> pattern() const
    {
        return pattern_;
    }

    int32_t get_time_limit() const {
        return matcher_->getTimeLimit();
    }

    void set_time_limit(int32_t time_limit_ms) {
        with_icu_error([&](UErrorCode* status){
            matcher_->setTimeLimit(time_limit_ms, *status);
        });
    }

    int32_t get_stack_limit() const {
        return matcher_->getStackLimit();
    }

    void set_stack_limit(int32_t stack_limit_bytes)
    {
        with_icu_error([&](UErrorCode* status){
            matcher_->setStackLimit(stack_limit_bytes, *status);
        });
    }

    void set_trace(bool do_trace) {
        matcher_->setTrace(do_trace);
    }

    void reset_preserve_region() {
        matcher_->resetPreserveRegion();
    }
};


class ICURegexPatternImpl: public EnableSharedFromThis<ICURegexPatternImpl> {
    std::unique_ptr<icu::RegexPattern> pattern_;

public:
    ICURegexPatternImpl(std::unique_ptr<icu::RegexPattern>&& pattern):
        pattern_(std::move(pattern))
    {
    }

public:

    ~ICURegexPatternImpl() noexcept {}

    uint32_t flags() const {
        return pattern_->flags();
    }

    U16String pattern() {
        return U16String(pattern_->pattern());
    }

    static LocalSharedPtr<ICURegexPatternImpl> compile(U16String pattern, uint32_t flags)
    {
        UErrorCode status = U_ZERO_ERROR;
        UParseError pe{};

        auto pattern_ptr = std::unique_ptr<icu::RegexPattern>(
             icu::RegexPattern::compile(pattern.to_icu_string(), flags, pe, status)
        );

        ICUParseException::assertOk(pattern, status, pe);

        return MakeLocalShared<detail::ICURegexPatternImpl>(std::move(pattern_ptr));
    }

    static LocalSharedPtr<ICURegexPatternImpl> compile(const CU16ProviderPtr& pattern, uint32_t flags, int32_t buffer_size)
    {
        UText ut = UTEXT_INITIALIZER;
        with_icu_error([&](UErrorCode* status0){
            utext_open_codepoint_accessor(&ut, pattern, buffer_size, status0);
        });

        UErrorCode status = U_ZERO_ERROR;
        UParseError pe{};
        auto pattern_ptr = std::unique_ptr<icu::RegexPattern>(
             icu::RegexPattern::compile(&ut, flags, pe, status)
        );

        if (U_FAILURE(status)) {
            ICUParseException::assertOk(to_u16string(&ut), status, pe);
        }

        return MakeLocalShared<detail::ICURegexPatternImpl>(std::move(pattern_ptr));
    }


    LocalSharedPtr<ICURegexMatcherImpl> matcher(U16String text)
    {
        auto utext_ptr = make_utext(std::move(text));
        auto matcher_ptr = with_icu_error([&](UErrorCode* status){
            return std::unique_ptr<RegexMatcher>(pattern_->matcher(*status));
        });

        matcher_ptr->reset(utext_ptr.get());

        return MakeLocalShared<ICURegexMatcherImpl>(shared_from_this(), std::move(matcher_ptr));
    }

    LocalSharedPtr<ICURegexMatcherImpl> matcher(const char16_t* input)
    {
        return make_matcher([&](UText* ut, UErrorCode* status){
             utext_openUChars(ut, castChars(input), -1, status);
        });
    }

    LocalSharedPtr<ICURegexMatcherImpl> matcher(const char16_t* input, size_t length)
    {
        return make_matcher([&](UText* ut, UErrorCode* status){
             utext_openUChars(ut, castChars(input), length, status);
        });
    }

    LocalSharedPtr<ICURegexMatcherImpl> matcher(const char* input)
    {
        return make_matcher([&](UText* ut, UErrorCode* status){
            utext_openUTF8(ut, castChars(input), -1, status);
        });
    }

    LocalSharedPtr<ICURegexMatcherImpl> matcher(const char* input, int32_t length)
    {
        return make_matcher([&](UText* ut, UErrorCode* status){
            utext_openUTF8(ut, castChars(input), length, status);
        });
    }


    LocalSharedPtr<ICURegexMatcherImpl> matcher(const CU16ProviderPtr& text, size_t buffer_size)
    {
        return make_matcher([&](UText* ut, UErrorCode* status){
            utext_open_codepoint_accessor(ut, text, buffer_size, status);
        });
    }


    std::vector<U16String> split(const U16String& text)
    {
        std::vector<U16String> results;

        split(text, [&](const ICUPatternMatch& match) {
            results.emplace_back(text.substring(match.start(), match.end() - match.start()));
        });

        return results;
    }

    void split(const CU16ProviderPtr& text, const ICURangeConsumerFn& consumer)
    {
        UText ut = UTEXT_INITIALIZER;
        with_icu_error([&](UErrorCode* status){
            utext_open_codepoint_accessor(&ut, text, 512, status);
        });

        UTextScope s0(&ut);
        split(&ut, consumer);
    }

    void split(const U16String& str, const ICURangeConsumerFn& consumer)
    {
        UText ut = UTEXT_INITIALIZER;
        with_icu_error([&](UErrorCode* status){
            utext_openUChars(&ut, str.data(), str.size(), status);
        });

        UTextScope s0(&ut);
        split(&ut, consumer);
    }

    void split(UText* ut, const ICURangeConsumerFn& consumer)
    {
        ICURegexMatcher matcher = create_matcher_ptr(ut);

        int64_t length = utext_nativeLength(ut);
        int64_t prev_ = 0;

        while (matcher.find())
        {
            consumer(ICUPatternMatch(prev_, matcher.start()));
            prev_ = matcher.end();
        }

        consumer(ICUPatternMatch(prev_, length));
    }

private:
    template <typename UTextOpenFn>
    LocalSharedPtr<ICURegexMatcherImpl> make_matcher(UTextOpenFn&& fn)
    {
        UText ut = UTEXT_INITIALIZER;
        with_icu_error([&](UErrorCode* status){
            return fn(&ut, status);
        });

        auto matcher_ptr = with_icu_error([&](UErrorCode* status){
            return std::unique_ptr<RegexMatcher>(pattern_->matcher(*status));
        });

        matcher_ptr->reset(&ut);

        return MakeLocalShared<ICURegexMatcherImpl>(shared_from_this(), std::move(matcher_ptr));
    }



    LocalSharedPtr<ICURegexMatcherImpl> create_matcher_ptr(UText* ut)
    {
        auto matcher_ptr = with_icu_error([&](UErrorCode* status){
            return std::unique_ptr<RegexMatcher>(pattern_->matcher(*status));
        });

        matcher_ptr->reset(ut);

        return MakeLocalShared<ICURegexMatcherImpl>(shared_from_this(), std::move(matcher_ptr));
    }

};
}





}
