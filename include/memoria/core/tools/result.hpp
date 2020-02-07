
// Copyright 2020 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/any.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <boost/variant2/variant.hpp>

#include <memory>
#include <exception>
#include <sstream>
#include <iostream>
#include <type_traits>

namespace memoria {

class MemoriaError {
public:
    MemoriaError() noexcept {}
    virtual ~MemoriaError() noexcept {}

    virtual void describe(std::ostream& out) const noexcept = 0;
    virtual const char* what() const noexcept = 0;
};

using MemoriaErrorPtr = std::unique_ptr<MemoriaError>;

class SimpleMemoriaError: public MemoriaError {
    U8String reason_;
public:
    SimpleMemoriaError(U8String reason) noexcept:
        reason_(std::move(reason))
    {}

    virtual void describe(std::ostream& out) const noexcept {
        out << reason_;
    }

    const U8String& reason() const & {
        return reason_;
    }

    U8String&& reason() && {
        return std::move(reason_);
    }

    virtual const char* what() const noexcept {
        return reason_.data();
    }
};

class ResultException: public std::exception {
    std::unique_ptr<MemoriaError> error_;

public:
    ResultException(std::unique_ptr<MemoriaError>&& error) noexcept :
        error_(std::move(error))
    {}

    const std::unique_ptr<MemoriaError>& error() const & {
        return error_;
    }

    std::unique_ptr<MemoriaError>& error() & {
        return error_;
    }

    std::unique_ptr<MemoriaError>&& error() && {
        return std::move(error_);
    }

    virtual const char* what() const noexcept
    {
        return error_->what();
    }
};

struct UnknownResultStatusException: MemoriaThrowable {};

enum class ResultStatus {
    RESULT_SUCCESS = 0, UNASSIGNED = 1, MEMORIA_ERROR = 2, EXCEPTION = 3
};


namespace detail {
    using ResultErrors = boost::variant2::variant<MemoriaErrorPtr, std::exception_ptr>;
    struct UnassignedResultValueType {};
}

template <typename T>
class Result;

using VoidResult = Result<void>;
using BoolResult = Result<bool>;
using Int32Result = Result<int32_t>;

template <typename T>
std::ostream& operator<<(std::ostream& out, const Result<T>& res) noexcept;

template <typename T>
std::ostream& operator<<(std::ostream& out, const Result<T*>& res) noexcept;

template <typename T>
class MMA_NODISCARD Result {
    using Variant = boost::variant2::variant<T, detail::UnassignedResultValueType, MemoriaErrorPtr, std::exception_ptr>;
    Variant variant_;

    struct ResultTag {};
    struct ErrorTag  {};

    template <typename... Args>
    Result(ResultTag, Args&&... args) noexcept:
        variant_(T{std::forward<Args>(args)...})
    {}

    template <typename U>
    Result(ErrorTag, U&& arg) noexcept:
        variant_(std::forward<U>(arg))
    {}

    template <typename>
    friend class Result;

public:
    using ValueType = T;

    Result() noexcept : variant_(detail::UnassignedResultValueType{}) {}

    Result(const Result&) = delete;
    Result(Result&&) noexcept = default;

    Result(T&& value) noexcept :
        variant_(std::move(value))
    {}

    Result(detail::ResultErrors&& other) noexcept :
        variant_(std::move(other))
    {}

    Result& operator=(Result&&) noexcept = default;

    template <typename... Args>
    static Result<T> of(Args&&... args) noexcept {
        return Result<T>(ResultTag{}, std::forward<Args>(args)...);
    }

    template <typename Arg>
    static Result<T> error(Arg&& arg) noexcept {
        return Result<T>(ErrorTag{}, std::forward<Arg>(arg));
    }

    template <typename... Arg>
    static Result<T> simple_memoria_error(Arg&&... args) noexcept {
        return Result<T>(ErrorTag{}, std::make_unique<SimpleMemoriaError>(std::forward<Arg>(args)...));
    }

    template <typename... Arg>
    static Result<T> make_error(const char* fmt, Arg&&... args) noexcept {
        return Result<T>(ErrorTag{}, std::make_unique<SimpleMemoriaError>(format_u8(fmt, std::forward<Arg>(args)...)));
    }

    template <typename... Arg>
    static detail::ResultErrors make_error_tr(const char* fmt, Arg&&... args) noexcept {
        return Result<T>(ErrorTag{}, std::make_unique<SimpleMemoriaError>(format_u8(fmt, std::forward<Arg>(args)...))).transfer_error();
    }

    bool is_ok() const noexcept {
        return variant_.index() == 0;
    }

    const T& get() const & noexcept {
        return *boost::variant2::get_if<T>(&variant_);
    }

    T& get() & noexcept {
        return *boost::variant2::get_if<T>(&variant_);
    }

    T&& get() && noexcept
    {
        return std::move(*boost::variant2::get_if<T>(&variant_));
    }

    const T& get_or_terminate() const & noexcept
    {
        const T* ptr = boost::variant2::get_if<T>(&variant_);

        if (ptr) {
            return *ptr;
        }

        std::stringstream ss;

        ss << "Getting value from error Result. Terminating.\n";
        ss << *this;

        terminate(ss.str().c_str());
    }

    T&& get_or_terminate() && noexcept
    {
        T* ptr = boost::variant2::get_if<T>(&variant_);

        if (ptr) {
            return std::move(*ptr);
        }

        std::stringstream ss;

        ss << "Getting value from error. Terminating.\n";
        ss << *this;

        terminate(ss.str().c_str());
    }

    template <typename Fn>
    T&& get_or(Fn&& accessor) && noexcept(accessor())
    {
        T* ptr = boost::variant2::get_if<T>(&variant_);

        if (ptr) {
            return std::move(*ptr);
        }

        return accessor();
    }

    const T& get_or_throw() const &
    {
        const T* ptr = boost::variant2::get_if<T>(&variant_);

        if (ptr) {
            return *ptr;
        }

        switch(status())
        {
            case ResultStatus::EXCEPTION: {
                std::rethrow_exception(
                    *boost::variant2::get_if<std::exception_ptr>(&variant_)
                );
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                throw ResultException(
                    *boost::variant2::get_if<MemoriaErrorPtr>(&variant_)
                );
            }
            default: break;
        }

        MMA_THROW(UnknownResultStatusException())
                << format_ex("Unknown result status value: {}", static_cast<int32_t>(status()));
    }

    T&& get_or_throw() &&
    {
        T* ptr = boost::variant2::get_if<T>(&variant_);

        if (ptr) {
            return std::move(*ptr);
        }

        switch(status())
        {
            case ResultStatus::EXCEPTION: {
                std::rethrow_exception(
                    *boost::variant2::get_if<std::exception_ptr>(&variant_)
                );
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                throw ResultException(
                    std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))
                );
            }
            default: break;
        }

        MMA_THROW(UnknownResultStatusException())
                << format_ex("Unknown result status value: {}", static_cast<int32_t>(status()));
    }

    void throw_if_error()
    {
        switch(status())
        {
            case ResultStatus::RESULT_SUCCESS: {
                return;
            }
            case ResultStatus::UNASSIGNED: {
                return;
            }
            case ResultStatus::EXCEPTION: {
                std::rethrow_exception(
                    *boost::variant2::get_if<std::exception_ptr>(&variant_)
                );
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                throw ResultException(
                    std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))
                );
            }
        }

        MMA_THROW(UnknownResultStatusException())
                << format_ex("Unknown result status value: {}", static_cast<int32_t>(status()));
    }

    void terminate_if_error() const noexcept
    {
        switch(status())
        {
            case ResultStatus::RESULT_SUCCESS: {
                return;
            }
            case ResultStatus::UNASSIGNED: {
                return;
            }
            default: break;
        }

        std::stringstream ss;

        ss << "Getting value from error. Terminating.\n";
        ss << *this;

        terminate(ss.str().c_str());
    }

    ResultStatus status() const noexcept {
        return static_cast<ResultStatus>(variant_.index());
    }

    detail::ResultErrors transfer_error() && noexcept
    {
        switch(status())
        {
        case ResultStatus::EXCEPTION: return std::move(*boost::variant2::get_if<std::exception_ptr>(&variant_));
        case ResultStatus::MEMORIA_ERROR: return std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_));
        default: terminate((SBuf() << "Unknown result status value: {}" << static_cast<int32_t>(status())).str().c_str());
        }
    }

    const MemoriaErrorPtr& memoria_error() const & noexcept {
        return boost::variant2::get<MemoriaErrorPtr>(variant_);
    }

    MemoriaErrorPtr& memoria_error() & noexcept {
        return boost::variant2::get<MemoriaErrorPtr>(variant_);
    }

    MemoriaErrorPtr&& memoria_error() && noexcept {
        return std::move(boost::variant2::get<MemoriaErrorPtr>(variant_));
    }


    const std::exception_ptr& exception() const & noexcept {
        return boost::variant2::get<std::exception_ptr>(variant_);
    }

    std::exception_ptr& exception() & noexcept {
        return boost::variant2::get<std::exception_ptr>(variant_);
    }

    std::exception_ptr&& exception() && noexcept {
        return std::move(boost::variant2::get<std::exception_ptr>(variant_));
    }
};


template <>
class MMA_NODISCARD Result<void> {
    using Variant = boost::variant2::variant<EmptyType, detail::UnassignedResultValueType, MemoriaErrorPtr, std::exception_ptr>;
    Variant variant_;

    struct ResultTag {};
    struct ErrorTag  {};

    template <typename>
    friend class Result;

    Result(ResultTag) noexcept:
        variant_(EmptyType{})
    {}

    template <typename U>
    Result(ErrorTag, U&& arg) noexcept:
        variant_(std::forward<U>(arg))
    {}

public:
    using ValueType = void;

    Result() noexcept: variant_(detail::UnassignedResultValueType{}) {}

    Result(const Result&) = delete;
    Result(Result&&) noexcept = default;

    Result(detail::ResultErrors&& other) noexcept :
        variant_(std::move(other))
    {}

    Result& operator=(Result&&) noexcept = default;


    static Result<void> of() noexcept {
        return Result<void>(ResultTag{});
    }

    template <typename Arg>
    static Result<void> error(Arg&& arg) noexcept {
        return Result<void>(ErrorTag{}, std::forward<Arg>(arg));
    }

    template <typename... Arg>
    static Result<void> simple_memoria_error(Arg&&... args) noexcept {
        return Result<void>(ErrorTag{}, std::make_unique<SimpleMemoriaError>(std::forward<Arg>(args)...));
    }

    template <typename... Arg>
    static Result<void> make_error(const char* fmt, Arg&&... args) noexcept {
        return Result<void>(ErrorTag{}, std::make_unique<SimpleMemoriaError>(format_u8(fmt, std::forward<Arg>(args)...)));
    }

    template <typename... Arg>
    static detail::ResultErrors make_error_tr(const char* fmt, Arg&&... args) noexcept {
        return Result<void>(ErrorTag{}, std::make_unique<SimpleMemoriaError>(format_u8(fmt, std::forward<Arg>(args)...))).transfer_error();
    }

    bool is_ok() const noexcept {
        return variant_.index() == 0;
    }

    void get_or_throw() {
        return throw_if_error();
    }

    void get_or_terminate() {
        return terminate_if_error();
    }

    void get() noexcept {
    }

    void throw_if_error()
    {
        switch(status())
        {
            case ResultStatus::RESULT_SUCCESS: {
                return;
            }
            case ResultStatus::UNASSIGNED: {
                return;
            }
            case ResultStatus::EXCEPTION: {
                std::rethrow_exception(
                    *boost::variant2::get_if<std::exception_ptr>(&variant_)
                );
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                (*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))->describe(std::cout);
                std::cout << std::endl;

                MMA_THROW(MemoriaThrowable()) << WhatCInfo("Forwarding memoria exception");

//                throw ResultException(
//                    std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))
//                );
            }
        }
    }

    void terminate_if_error() const noexcept
    {
        switch(status())
        {
            case ResultStatus::RESULT_SUCCESS: {
                return;
            }
            case ResultStatus::UNASSIGNED: {
                return;
            }
            default: break;
        }

        std::stringstream ss;

        ss << "Getting value from error. Terminating.\n";
        //ss << *this;

        terminate(ss.str().c_str());
    }

    const MemoriaErrorPtr& memoria_error() const & noexcept {
        return boost::variant2::get<MemoriaErrorPtr>(variant_);
    }

    MemoriaErrorPtr& memoria_error() & noexcept {
        return boost::variant2::get<MemoriaErrorPtr>(variant_);
    }

    MemoriaErrorPtr&& memoria_error() && noexcept {
        return std::move(boost::variant2::get<MemoriaErrorPtr>(variant_));
    }


    const std::exception_ptr& exception() const & noexcept {
        return boost::variant2::get<std::exception_ptr>(variant_);
    }

    std::exception_ptr& exception() & noexcept {
        return boost::variant2::get<std::exception_ptr>(variant_);
    }

    std::exception_ptr&& exception() && noexcept {
        return std::move(boost::variant2::get<std::exception_ptr>(variant_));
    }

    ResultStatus status() const noexcept {
        return static_cast<ResultStatus>(variant_.index());
    }

    detail::ResultErrors transfer_error() && noexcept
    {
        switch(status())
        {
        case ResultStatus::EXCEPTION: return std::move(*boost::variant2::get_if<std::exception_ptr>(&variant_));
        case ResultStatus::MEMORIA_ERROR: return std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_));
        default: terminate(format_u8("Unknown result status value: {}", static_cast<int32_t>(status())).data());
        }
    }
};

namespace detail {

    template <typename T>
    struct IsResultH: HasValue<bool, false> {};

    template <typename T>
    struct IsResultH<Result<T>>: HasValue<bool, true> {};

    template <typename T>
    struct IsVoidResultH: HasValue<bool, false> {};

    template <>
    struct IsVoidResultH<void>: HasValue<bool, true> {};


    template <typename Fn, typename Rtn = std::result_of_t<Fn>>
    struct ResultOfFn: HasType<Result<Rtn>> {};

    template <typename Fn, typename T>
    struct ResultOfFn<Fn, Result<T>>: HasType<Result<T>> {};

    template <typename T>
    Result<T> wrap_fn_result(T&& value) noexcept {
        return Result<T>::of(std::forward<T>(value));
    }

    template <typename T>
    Result<T> wrap_fn_result(Result<T>&& value) noexcept {
        return std::move(value);
    }

    template <typename T>
    void print_error(std::ostream&out, const Result<T>& result) noexcept
    {
        ResultStatus status = result.status();
        if (status == ResultStatus::UNASSIGNED) {
            out << "<Unassigned>";
        }
        else if (status == ResultStatus::MEMORIA_ERROR) {
            result.memoria_error()->describe(out);
        }
        else if (status == ResultStatus::EXCEPTION) {
            try {
                std::rethrow_exception(result.exception());
            }
            catch (MemoriaThrowable& mth) {
                mth.dump(out);
            }
            catch (std::exception& ex) {
                out << boost::diagnostic_information(ex);
            }
            catch (boost::exception& ex) {
                out << boost::diagnostic_information(ex);
            }
            catch (...) {
                out << "Unknown Exception";
            }
        }
    }

    template <typename T>
    auto print_result(std::ostream&out, const Result<T>& result, int) noexcept -> decltype(operator<<(out, result.get()))
    {
        if (result.is_ok())
        {
            std::stringstream buf;
            buf << result.get_or_terminate();
            out << buf.str();
        }
        else {
            print_error(out, result);
        }

        return out;
    }

    template <typename T>
    auto print_result(std::ostream&out, const Result<T>& result, int) noexcept -> decltype(out.operator<<(result.get()))
    {
        if (result.is_ok())
        {
            out << result.get_or_terminate();
        }
        else {
            print_error(out, result);
        }

        return out;
    }

    template <typename T>
    std::ostream& print_result(std::ostream&out, const Result<T>& result, ...) noexcept
    {
        if (result.is_ok())
        {
            if (std::is_same<T, void>::value) {
                out << "<Void>";
            }
            else {
                out << "Unprintable: " << TypeNameFactory<T>::name();
            }
        }
        else {
            print_error(out, result);
        }

        return out;
    }
}

template <typename Fn>
std::enable_if_t<
    !detail::IsVoidResultH<std::result_of_t<Fn()>>::Value,
    typename detail::ResultOfFn<Fn()>::Type
>
wrap_throwing(Fn&& fn) noexcept
{
    try {
        return detail::wrap_fn_result(fn());
    }
    catch (...) {
        using RtnT = typename detail::ResultOfFn<Fn()>::Type::ValueType;
        return Result<RtnT>::error(std::current_exception());
    }
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const Result<T>& res) noexcept
{
    return detail::print_result(out, res, 0);
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const Result<T*>& res) noexcept
{
    if (res.is_ok()) {
        out << res.get();
    }
    else {
        detail::print_error(out, res);
    }

    return out;
}



template <typename Fn>
std::enable_if_t<
    detail::IsVoidResultH<std::result_of_t<Fn()>>::Value,
    typename detail::ResultOfFn<Fn()>::Type
>
wrap_throwing(Fn&& fn) noexcept
{
    using RtnT = typename detail::ResultOfFn<Fn()>::Type::ValueType;
    try {
        fn();
        return Result<RtnT>::of();
    }
    catch (...) {
        return Result<RtnT>::error(std::current_exception());
    }
}

#define MEMORIA_RETURN_IF_ERROR(ResultVal)   \
do {                                         \
    if (MMA_UNLIKELY(!ResultVal.is_ok())) {  \
        return std::move(ResultVal).transfer_error(); \
    }                                        \
} while (0)

#define MEMORIA_RETURN_IF_ERROR_FN(FnCall)   \
do {                                         \
    auto res0 = FnCall;                      \
    if (MMA_UNLIKELY(!res0.is_ok())) {       \
        return std::move(res0).transfer_error(); \
    }                                        \
} while (0)

#define MEMORIA_TRY_VOID(FnCall)             \
do {                                         \
    auto res0 = FnCall;                      \
    if (MMA_UNLIKELY(!res0.is_ok())) {       \
        return std::move(res0).transfer_error(); \
    }                                        \
} while(0)

#define MEMORIA_TRY(VarName, Code)                  \
    auto VarName##_result = Code;                  \
    if (MMA_UNLIKELY(!VarName##_result.is_ok())) return std::move(VarName##_result).transfer_error(); \
    auto& VarName = VarName##_result.get()

}
