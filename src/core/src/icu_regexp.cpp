
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

#include "icu_regexp_impl.hpp"

namespace memoria {


ICURegexMatcher::~ICURegexMatcher() noexcept {}

bool ICURegexMatcher::matches() {
    return ptr_->matches();
}

bool ICURegexMatcher::matches(int64_t start) {
    return ptr_->matches(start);
}

bool ICURegexMatcher::looking_at() {
    return ptr_->looking_at();
}

bool ICURegexMatcher::looking_at(int64_t start)
{
    return ptr_->looking_at(start);
}

bool ICURegexMatcher::find()
{
    return ptr_->find();
}

bool ICURegexMatcher::find(int64_t start)
{
    return ptr_->find(start);
}

U16String ICURegexMatcher::group() const {
    return ptr_->group();
}

U16String ICURegexMatcher::group(int32_t group_num) const {
    return ptr_->group(group_num);
}

int32_t ICURegexMatcher::groups_count() const {
    return ptr_->groups_count();
}

int64_t ICURegexMatcher::start() const {
    return ptr_->start();
}

int64_t ICURegexMatcher::start(int32_t group) const {
    return ptr_->start(group);
}

int64_t ICURegexMatcher::end() const {
    return ptr_->end();
}

int64_t ICURegexMatcher::end(int32_t group) const {
    return ptr_->end(group);
}

ICURegexMatcher& ICURegexMatcher::reset(){
    ptr_->reset();
    return *this;
}

ICURegexMatcher& ICURegexMatcher::reset(int64_t start){
    ptr_->reset(start);
    return *this;
}

U16String ICURegexMatcher::input() const {
    return ptr_->input();
}

CU16ProviderPtr ICURegexMatcher::input_text() const {
    return ptr_->input_text();
}

ICURegexMatcher& ICURegexMatcher::region(int64_t start, int64_t end) {
    ptr_->region(start, end);
    return *this;
}

ICURegexMatcher& ICURegexMatcher::region(int64_t region_start, int64_t region_end, int64_t start_index)
{
    ptr_->groups_count();
    return *this;
}

int64_t ICURegexMatcher::region_start() const {
    return ptr_->region_start();
}

int64_t ICURegexMatcher::region_end() const {
    return ptr_->region_end();
}

bool ICURegexMatcher::has_transparent_bounds() const {
    return ptr_->has_anchoring_bounds();
}

ICURegexMatcher& ICURegexMatcher::use_transaprent_bounds(bool use_it) {
    ptr_->use_transaprent_bounds(use_it);
    return *this;
}


bool ICURegexMatcher::has_anchoring_bounds() const {
    return ptr_->has_anchoring_bounds();
}

ICURegexMatcher& ICURegexMatcher::use_anchoring_bounds(bool use_it) {
    ptr_->use_anchoring_bounds(use_it);
    return *this;
}

bool ICURegexMatcher::hit_end() const {
    return ptr_->hit_end();
}

bool ICURegexMatcher::require_end() const {
    return ptr_->require_end();
}

ICURegexPattern ICURegexMatcher::pattern() const {
    return ptr_->pattern();
}

int32_t ICURegexMatcher::get_time_limit() const {
    return ptr_->get_time_limit();
}

ICURegexMatcher& ICURegexMatcher::set_time_limit(int32_t time_limit_ms) {
    ptr_->set_time_limit(time_limit_ms);
    return *this;
}

int32_t ICURegexMatcher::get_stack_limit() const {
    return ptr_->get_stack_limit();
}

ICURegexMatcher& ICURegexMatcher::set_stack_limit(int32_t stack_limit_bytes){
    ptr_->set_stack_limit(stack_limit_bytes);
    return *this;
}

ICURegexMatcher& ICURegexMatcher::set_trace(bool do_trace) {
    ptr_->set_trace(do_trace);
    return *this;
}

void ICURegexMatcher::reset_preserve_region() {
    ptr_->reset_preserve_region();
}




ICURegexPattern::~ICURegexPattern() noexcept {}

U16String ICURegexPattern::pattern() const {
    return pattern_->pattern();
}

uint32_t ICURegexPattern::flags() const {
    return pattern_->flags();
}

ICURegexMatcher ICURegexPattern::matcher(U16String input) {
    return pattern_->matcher(input);
}

ICURegexMatcher ICURegexPattern::matcher(const CU16ProviderPtr& input, int32_t buffer_size) {
    return pattern_->matcher(input, buffer_size);
}


ICURegexMatcher ICURegexPattern::matcher(const char16_t* input) {
    return pattern_->matcher(input);
}

ICURegexMatcher ICURegexPattern::matcher(const char16_t* input, int32_t length) {
    return pattern_->matcher(input, length);
}

ICURegexMatcher ICURegexPattern::matcher(const char* input) {
    return pattern_->matcher(input);
}

ICURegexMatcher ICURegexPattern::matcher(const char* input, int32_t length) {
    return pattern_->matcher(input, length);
}



ICURegexPattern ICURegexPattern::compile(U16String pattern, uint32_t flags) {
    return _::ICURegexPatternImpl::compile(pattern, flags);
}

ICURegexPattern ICURegexPattern::compile(const CU16ProviderPtr& pattern, uint32_t flags, int32_t buffer_size) {
    return _::ICURegexPatternImpl::compile(pattern, flags, buffer_size);
}


std::vector<U16String> ICURegexPattern::split(const U16String& str) {
    return pattern_->split(str);
}

std::vector<U8String> ICURegexPattern::split(const U8String& str)
{
    std::vector<U16String> pp = pattern_->split(str.to_u16());

    std::vector<U8String> rr;

    for (U16String token: pp) {
        rr.emplace_back(token.to_u8());
    }

    return rr;
}


void ICURegexPattern::split(const CU16ProviderPtr& text, const ICURangeConsumerFn& consumer)
{
    return pattern_->split(text, consumer);
}

void ICURegexPattern::split(const U16String& text, const ICURangeConsumerFn& consumer)
{
    return pattern_->split(text, consumer);
}



}
