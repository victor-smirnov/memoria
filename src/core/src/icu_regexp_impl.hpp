
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

#include <memoria/v1/core/tools/regexp/icu_regexp.hpp>

namespace memoria {
namespace v1 {
namespace _ {

class ICURegexMatcherImpl {

    std::shared_ptr<ICURegexPatternImpl> pattern_;
    std::unique_ptr<RegexMatcher> matcher_;

public:
    ICURegexMatcherImpl(const std::shared_ptr<ICURegexPatternImpl>& pattern, std::unique_ptr<RegexMatcher>&& matcher):
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

            return to_u16string(closeable_ptr, start, start + group_len);
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

            return to_u16string(closeable_ptr, start, start + group_len);
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

        return to_u16string(closeable_ptr);
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

        return wrap_utext(std::move(closeable_ptr));
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

    std::shared_ptr<ICURegexPatternImpl> pattern() const
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


class ICURegexPatternImpl: public std::enable_shared_from_this<ICURegexPatternImpl> {
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

    static std::shared_ptr<ICURegexPatternImpl> compile(const U16String &pattern, uint32_t flags)
    {
        UErrorCode status = U_ZERO_ERROR;
        UParseError pe{};

        auto pattern_ptr = std::unique_ptr<icu::RegexPattern>(
             icu::RegexPattern::compile(pattern.to_icu_string(), flags, pe, status)
        );

        ICUParseException::assertOk(pattern, status, pe);

        return std::make_shared<_::ICURegexPatternImpl>(std::move(pattern_ptr));
    }

    std::shared_ptr<ICURegexMatcherImpl> matcher(const U16String& text)
    {
        auto utext_ptr = make_utext(text);
        auto matcher_ptr = with_icu_error([&](UErrorCode* status){
            return std::unique_ptr<RegexpMatcher>(pattern_->matcher(text.data(), *status));
        });

        return std::shared_ptr<ICURegexMatcherImpl>(matcher_ptr);
    }

    std::shared_ptr<ICURegexMatcherImpl> matcher(const CU16ProviderPtr& text) {
        return std::shared_ptr<ICURegexMatcherImpl>();
    }
};
}





}
}
