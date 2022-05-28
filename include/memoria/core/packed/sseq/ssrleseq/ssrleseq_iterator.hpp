
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


#pragma once

#include <memoria/core/tools/bitmap_select.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/core/ssrle/ssrle.hpp>

#include <vector>
#include <functional>

namespace memoria {

namespace ssrleseq {

template <typename Seq>
class SymbolsRunIterator {
    using RunT      = typename Seq::SymbolsRunT;
    using CodeUnitT = typename Seq::CodeUnitT;
    using RunTraits = typename Seq::RunTraits;

    Span<const CodeUnitT> span_;

    size_t idx_;
    size_t idx_next_;

public:
    SymbolsRunIterator() noexcept: span_(), idx_(), idx_next_() {}
    SymbolsRunIterator(Span<const CodeUnitT> span) noexcept:
        span_(span), idx_(), idx_next_()
    {}

    SymbolsRunIterator(Span<const CodeUnitT> span, size_t idx) noexcept:
        span_(span), idx_(idx), idx_next_()
    {}

    RunT get() noexcept {
        RunT run;
        idx_next_ = idx_ + RunTraits::decode_unit_to(span_.data() + idx_, run);
        return run;
    }

    bool is_eos() const noexcept {
        return idx_ >= span_.size();
    }

    void next() noexcept {
        idx_ = idx_next_;
    }

    void next(size_t skip) noexcept {
        idx_ += skip;
    }

    size_t idx() const noexcept {
        return idx_;
    }

    Span<const CodeUnitT> span() const noexcept {
        return span_;
    }

    std::vector<RunT> as_vector() noexcept
    {
        std::vector<RunT> rr;
        while (!is_eos())
        {
            RunT run = this->get();

            if (run) {
                rr.push_back(run);
                next();
            }
            else if (run.is_padding()) {
                next(run.run_length());
            }
            else {
                break;
            }
        }

        return rr;
    }


    size_t read_to(std::vector<RunT>& sink) noexcept
    {
        size_t idx0 = idx_;

        while (!is_eos())
        {
            RunT run = this->get();

            if (run) {
                sink.push_back(run);
                next();
            }
            else if (run.is_padding()) {
                next(run.run_length());
            }
            else {
                break;
            }
        }

        return idx_ - idx0;
    }

    size_t read_to(ArenaBuffer<RunT>& sink) noexcept
    {
        size_t idx0 = idx_;

        while (!is_eos())
        {
            RunT run = this->get();

            if (run) {
                sink.append_value(run);
                next();
            }
            else if (run.is_padding()) {
                next(run.run_length());
            }
            else {
                break;
            }
        }

        return idx_ - idx0;
    }


    auto for_each(std::function<void (const RunT& run)> fn)
    {
        while (!is_eos())
        {
            RunT run = this->get();

            if (run) {
                fn(run);
                next();
            }
            else if (run.is_padding()) {
                next(run.run_length());
            }
            else {
                break;
            }
        }
    }

    auto for_each(std::function<void (const RunT& run, size_t)> fn)
    {
        while (!is_eos())
        {
            RunT run = this->get();

            if (run) {
                fn(run, idx_);
                next();
            }
            else if (run.is_padding()) {
                next(run.run_length());
            }
            else {
                break;
            }
        }
    }

    template <typename Fn>
    void do_while(Fn&& fn)
    {
        while (!is_eos())
        {
            RunT run = this->get();

            if (run) {
                if (fn(run)) {
                    next();
                }
                else {
                    return;
                }
            }
            else if (run.is_padding()) {
                next(run.run_length());
            }
            else {
                break;
            }
        }
    }

    template <typename Fn>
    void do_while1(Fn&& fn)
    {
        while (!is_eos())
        {
            RunT run = this->get();

            if (run) {
                if (fn(run, idx_)) {
                    next();
                }
                else {
                    return;
                }
            }
            else if (run.is_padding()) {
                next(run.run_length());
            }
            else {
                break;
            }
        }
    }
};

}}
