
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/arena_buffer.hpp>


#include <ostream>

namespace memoria {
namespace hermes {

class DocView;

class DumpFormatState {
    const char* space_;

    const char* nl_start_;
    const char* nl_middle_;
    const char* nl_end_;

    size_t indent_size_;
    size_t current_indent_;

public:
    DumpFormatState(
            const char* space,
            const char* nl_start,
            const char* nl_middle,
            const char* nl_end,
            size_t indent_size
    ):
        space_(space),
        nl_start_(nl_start),
        nl_middle_(nl_middle),
        nl_end_(nl_end),
        indent_size_(indent_size),
        current_indent_(0)
    {}

    DumpFormatState(): DumpFormatState(" ", "\n", "\n", "\n", 2) {}

    static DumpFormatState no_indent() {
        return DumpFormatState("", "", "", "", 0);
    }

    static DumpFormatState simple() {
        return DumpFormatState("", "", " ", "", 0);
    }

    const char* space() const {return space_;}

    const char* nl_start() const {return nl_start_;}
    const char* nl_middle() const {return nl_middle_;}
    const char* nl_end() const {return nl_end_;}

    size_t indent_size() const {return indent_size_;}
    size_t current_indent() const {return current_indent_;}

    void push() {
        current_indent_ += indent_size_;
    }

    void pop() {
        current_indent_ -= indent_size_;
    }

    void make_indent(std::ostream& out) const {
        for (size_t c = 0; c < current_indent_; c++) {
            out << space_;
        }
    }
};

class DumpState {
public:
    DumpState(const DocView& doc) {}
};

class StringEscaper {
    ArenaBuffer<U8StringView::value_type> buffer_;
public:

    bool has_quotes(U8StringView str) const noexcept
    {
        for (auto& ch: str) {
            if (ch == '\'') return true;
        }

        return false;
    }

    U8StringView escape_quotes(const U8StringView& str)
    {
        if (!has_quotes(str)) {
            return str;
        }
        else {
            buffer_.clear();

            for (auto& ch: str)
            {
                if (MMA_UNLIKELY(ch == '\''))
                {
                    buffer_.append_value('\\');
                }

                buffer_.append_value(ch);
            }

            buffer_.append_value(0);

            return U8StringView(buffer_.data(), buffer_.size() - 1);
        }
    }

    void reset()
    {
        if (buffer_.size() <= 1024*16) {
            buffer_.clear();
        }
        else {
            buffer_.reset();
        }
    }

    static StringEscaper& current();
};



template <typename AccessorType>
class RandomAccessIterator: public boost::iterator_facade<
        RandomAccessIterator<AccessorType>,
        const typename AccessorType::ViewType,
        std::random_access_iterator_tag,
        const typename AccessorType::ViewType
> {
    using ViewType = typename AccessorType::ViewType;

    size_t pos_;
    size_t size_;
    AccessorType accessor_;

    using Iterator = RandomAccessIterator;

public:
    RandomAccessIterator() : pos_(), size_(), accessor_() {}

    RandomAccessIterator(AccessorType accessor, size_t pos, size_t size) :
        pos_(pos), size_(size), accessor_(accessor)
    {}

    size_t size() const noexcept {return size_;}
    size_t pos() const noexcept {return pos_;}

    bool is_end() const noexcept {return pos_ >= size_;}
    operator bool() const noexcept {return !is_end();}

private:
    friend class boost::iterator_core_access;

    ViewType dereference() const  {
        return accessor_.get(pos_);
    }

    bool equal(const RandomAccessIterator& other) const  {
        return accessor_ == other.accessor_ && pos_ == other.pos_;
    }

    void increment() {
        pos_ += 1;
    }

    void decrement() {
        pos_ -= 1;
    }

    void advance(int64_t n)  {
        pos_ += n;
    }

    ptrdiff_t distance_to(const RandomAccessIterator& other) const
    {
        ptrdiff_t res = static_cast<ptrdiff_t>(other.pos_) - static_cast<ptrdiff_t>(pos_);
        return res;
    }
};


template <typename AccessorType>
class ForwardIterator: public boost::iterator_facade<
        ForwardIterator<AccessorType>,
        const typename AccessorType::ViewType,
        std::forward_iterator_tag,
        const typename AccessorType::ViewType
> {
    using ViewType = typename AccessorType::ViewType;

    AccessorType accessor_;

    using Iterator = ForwardIterator;

public:
    ForwardIterator() : accessor_() {}

    ForwardIterator(AccessorType accessor) : accessor_(accessor)
    {}

private:
    friend class boost::iterator_core_access;

    ViewType dereference() const  {
        return accessor_.current();
    }

    bool equal(const ForwardIterator& other) const  {
        return accessor_ == other.accessor_;
    }

    void increment() {
        accessor_.next();
    }
};

}}
