
// Copyright 2020-2022 Victor Smirnov
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
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/strings/u8_string.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <boost/variant2/variant.hpp>

#include <fmt/format.h>

#include <memory>
#include <exception>
#include <sstream>
#include <iostream>
#include <type_traits>

namespace memoria {

enum class ErrorCategory {GENERIC, PACKED, EXCEPTION};

class MemoriaError {
    ErrorCategory error_category_;
public:
    MemoriaError(ErrorCategory error_category) noexcept: error_category_(error_category)  {}

    virtual ~MemoriaError() noexcept {}

    ErrorCategory error_category() const noexcept {
        return error_category_;
    }

    virtual void describe(std::ostream& out) const noexcept = 0;
    virtual const U8String what() const noexcept = 0;

    virtual void release() noexcept = 0;
};

void release(MemoriaError* error) noexcept;

using MemoriaErrorPtr = std::unique_ptr<MemoriaError, std::function<void(MemoriaError*)>>;

template <typename ErrorType, typename... Args>
MemoriaErrorPtr make_memoria_error(Args&&... args) {
    return MemoriaErrorPtr(new ErrorType{std::forward<Args>(args)...}, release);
}



class GenericMemoriaError: public MemoriaError {
    const char* source_;
    U8String reason_;
public:
    GenericMemoriaError(U8String reason) noexcept:
        MemoriaError(ErrorCategory::GENERIC),
        source_("NONE"),
        reason_(std::move(reason))
    {}

    GenericMemoriaError(const char* source, U8String reason) noexcept:
        MemoriaError(ErrorCategory::GENERIC),
        source_(source),
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

    virtual const U8String what() const noexcept {
        return reason_;
    }

    virtual void release() noexcept {
        delete this;
    }

    const char* source() noexcept {
        return source_;
    }
};

class PackedOOMError: public MemoriaError {
public:
    PackedOOMError() noexcept: MemoriaError(ErrorCategory::PACKED) {}

    virtual void release() noexcept {
        delete this;
    }

    virtual void describe(std::ostream& out) const noexcept
    {
        out << what();
    }

    virtual const U8String what() const noexcept {
        return "PackedOOMError";
    }
};

class WrappedExceptionMemoriaError: public MemoriaError {
    std::exception_ptr ptr_;
public:
    WrappedExceptionMemoriaError(std::exception_ptr&& ptr) noexcept:
        MemoriaError(ErrorCategory::EXCEPTION),
        ptr_(std::move(ptr))
    {}

    void release() noexcept;
    void describe(std::ostream& out) const noexcept;
    const U8String what() const noexcept;

    std::exception_ptr& ptr();

    [[noreturn]] void rethrow() const;

    static MemoriaErrorPtr create(std::exception_ptr&& ptr);
};


class ResultException: public std::exception {
    MemoriaErrorPtr error_;
    mutable std::unique_ptr<U8String> what_;

public:
    ResultException(MemoriaErrorPtr&& error) noexcept :
        error_(std::move(error))
    {}

    const MemoriaErrorPtr& error() const & {
        return error_;
    }

    MemoriaErrorPtr& error() & {
        return error_;
    }

    MemoriaErrorPtr&& error() && {
        return std::move(error_);
    }

    virtual const char* what() const noexcept
    {
        if (!what_)
        {
            what_ = std::make_unique<U8String>(error_->what());
        }

        return what_->data();
    }
};

struct UnknownResultStatusException: MemoriaThrowable {};
struct ResultUninitializedException: MemoriaThrowable {};

namespace detail {
    struct UninitializedType {};
}


enum class ResultStatus {
    RESULT_SUCCESS = 0, MEMORIA_ERROR = 1, RESULT_UNINITIALIZED = 2
};


namespace detail {

struct ResultErrors {
    using ErrorsT = boost::variant2::variant<MemoriaErrorPtr, detail::UninitializedType>;

    ErrorsT error;

    [[noreturn]] void do_throw();
};

}

using MaybeError = Optional<detail::ResultErrors>;


template <typename... Args>
detail::ResultErrors make_generic_error(const char* fmt, Args&&... args) noexcept {
    return detail::ResultErrors{make_memoria_error<GenericMemoriaError>(format_u8(fmt, std::forward<Args>(args)...))};
}

template <typename... Args>
detail::ResultErrors make_generic_error_with_source(const char* source, const char* fmt, Args&&... args) noexcept {
    return detail::ResultErrors{make_memoria_error<GenericMemoriaError>(source, format_u8(fmt, std::forward<Args>(args)...))};
}

#define MEMORIA_MAKE_GENERIC_ERROR(FMT, ...) ::memoria::make_generic_error_with_source(MMA_SRC, FMT, ##__VA_ARGS__)


inline detail::ResultErrors make_packed_oom_error() noexcept {
    return detail::ResultErrors{make_memoria_error<PackedOOMError>()};
}

#define MEMORIA_MAKE_PACKED_OOM_ERROR() ::memoria::make_packed_oom_error()


template <typename T>
class Result;

using VoidResult  = Result<void>;
using BoolResult  = Result<bool>;
using Int32Result = Result<int32_t>;
using Int64Result = Result<int64_t>;
using UInt64Result = Result<uint64_t>;
using SizeTResult = Result<size_t>;

template <typename T>
std::ostream& operator<<(std::ostream& out, const Result<T>& res) noexcept;

template <typename T>
std::ostream& operator<<(std::ostream& out, const Result<T*>& res) noexcept;


template <typename T>
class MMA_NODISCARD Result {
    using Variant = boost::variant2::variant<T, MemoriaErrorPtr, detail::UninitializedType>;
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

    Result() noexcept:
        variant_(detail::UninitializedType{})
    {}

    Result(const Result&) = delete;
    Result(Result&&) noexcept = default;

    Result(T&& value) noexcept :
        variant_(std::move(value))
    {}

    Result(detail::ResultErrors&& other) noexcept :
        variant_(std::move(other.error))
    {}

    Result& operator=(Result&&) noexcept = default;

    template <typename... Args>
    static Result<T> of(Args&&... args) noexcept {
        return Result<T>(ResultTag{}, std::forward<Args>(args)...);
    }

    bool is_packed_error() const noexcept {
        return memoria_error()->error_category() == ErrorCategory::PACKED;
    }

    bool is_ok() const noexcept {
        return variant_.index() == 0;
    }

    bool is_error() const noexcept {
        return variant_.index() != 0;
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
    T&& get_or(Fn&& accessor) && noexcept(std::declval<Fn>())
    {
        T* ptr = boost::variant2::get_if<T>(&variant_);

        if (ptr) {
            return std::move(*ptr);
        }

        return accessor();
    }

    T& get_or_throw() &
    {
        T* ptr = boost::variant2::get_if<T>(&variant_);

        if (ptr) {
            return *ptr;
        }

        switch(status())
        {
            case ResultStatus::RESULT_UNINITIALIZED:
            {
                throw ResultUninitializedException{};
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                if (memoria_error()->error_category() == ErrorCategory::EXCEPTION)
                {
                    WrappedExceptionMemoriaError* err = static_cast<WrappedExceptionMemoriaError*>(memoria_error().get());
                    err->rethrow();
                }

                throw ResultException(
                    std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))
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
            case ResultStatus::RESULT_UNINITIALIZED:
            {
                throw ResultUninitializedException{};
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                if (memoria_error()->error_category() == ErrorCategory::EXCEPTION)
                {
                    WrappedExceptionMemoriaError* err = static_cast<WrappedExceptionMemoriaError*>(memoria_error().get());
                    err->rethrow();
                }

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
            case ResultStatus::RESULT_UNINITIALIZED:
            {
                throw ResultUninitializedException{};
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                if (memoria_error()->error_category() == ErrorCategory::EXCEPTION)
                {
                    WrappedExceptionMemoriaError* err = static_cast<WrappedExceptionMemoriaError*>(memoria_error().get());
                    err->rethrow();
                }

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
        case ResultStatus::RESULT_UNINITIALIZED: return detail::ResultErrors{std::move(*boost::variant2::get_if<detail::UninitializedType>(&variant_))};
        case ResultStatus::MEMORIA_ERROR: return detail::ResultErrors{std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))};
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
    using Variant = boost::variant2::variant<EmptyType, MemoriaErrorPtr, detail::UninitializedType>;

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

    Result() noexcept:
        variant_(detail::UninitializedType{})
    {}


    Result(const Result&) = delete;
    Result(Result&&) noexcept = default;

    Result(detail::ResultErrors&& other) noexcept :
        variant_(std::move(other.error))
    {}

    Result& operator=(Result&&) noexcept = default;


    static Result<void> of() noexcept {
        return Result<void>(ResultTag{});
    }

    bool is_packed_error() const noexcept {
        return memoria_error()->error_category() == ErrorCategory::PACKED;
    }

    bool is_ok() const noexcept {
        return variant_.index() == 0;
    }

    bool is_error() const noexcept {
        return variant_.index() != 0;
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
            case ResultStatus::RESULT_UNINITIALIZED: {
                throw ResultUninitializedException();
            }
            case ResultStatus::MEMORIA_ERROR:
            {
                if (memoria_error()->error_category() == ErrorCategory::EXCEPTION)
                {
                    WrappedExceptionMemoriaError* err = static_cast<WrappedExceptionMemoriaError*>(memoria_error().get());
                    err->rethrow();
                }

                throw ResultException(
                    std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))
                );
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
            default: break;
        }

        std::stringstream ss;

        ss << "Getting value from error. Terminating.\n";
        ss << *this;

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

    ResultStatus status() const noexcept {
        return static_cast<ResultStatus>(variant_.index());
    }

    detail::ResultErrors transfer_error() && noexcept
    {
        switch(status())
        {
        case ResultStatus::RESULT_UNINITIALIZED: return detail::ResultErrors{std::move(*boost::variant2::get_if<detail::UninitializedType>(&variant_))};
        case ResultStatus::MEMORIA_ERROR: return detail::ResultErrors{std::move(*boost::variant2::get_if<MemoriaErrorPtr>(&variant_))};
        default: terminate(format_u8("Unknown result status value: {}", static_cast<int32_t>(status())).data());
        }
    }
};

template <typename T>
bool isFail(const Result<T>& res) noexcept {
    return res.is_error();
}

inline bool isFail(const MaybeError& res) noexcept {
    return res.is_initialized();
}

inline bool isFail(const detail::ResultErrors&) noexcept {
    return true;
}



namespace detail {

    template <typename T>
    struct IsResultH: HasValue<bool, false> {};

    template <typename T>
    struct IsResultH<Result<T>>: HasValue<bool, true> {};

    template <typename T>
    struct IsVoidResultH: HasValue<bool, false> {};

    template <>
    struct IsVoidResultH<void>: HasValue<bool, true> {};


    template <typename T>
    struct ResultOfFn: HasType<Result<T>> {};

    template <typename T>
    struct ResultOfFn<Result<T>>: HasType<Result<T>> {};

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
        if (status == ResultStatus::MEMORIA_ERROR) {
            result.memoria_error()->describe(out);
        }
        else if (status == ResultStatus::RESULT_UNINITIALIZED) {
            out << "[[Uninitialized]]";
        }
        else {
            out << "[[Not an error]]";
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

    template <typename T>
    ResultErrors propagate_errors(Result<T>&& res) {
        return std::move(res).transfer_error();
    }

    inline ResultErrors propagate_errors(MaybeError&& err) {
        return std::move(err.get());
    }
}


template <typename T>
bool IsResultT = detail::IsResultH<T>::Value;

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
    !detail::IsVoidResultH<std::invoke_result_t<Fn>>::Value,
    typename detail::ResultOfFn<std::invoke_result_t<Fn>>::Type
>
wrap_throwing(Fn&& fn) noexcept
{
    try {
        return detail::wrap_fn_result(fn());
    }
    catch (...) {
        return detail::ResultErrors{WrappedExceptionMemoriaError::create(std::current_exception())};
    }
}


template <typename Fn>
std::enable_if_t<
    detail::IsVoidResultH<std::invoke_result_t<Fn>>::Value,
    typename detail::ResultOfFn<std::invoke_result_t<Fn>>::Type
>
wrap_throwing(Fn&& fn) noexcept
{
    using RtnT = typename detail::ResultOfFn<std::invoke_result_t<Fn>>::Type::ValueType;
    try {
        fn();
        return Result<RtnT>::of();
    }
    catch (...) {
        return detail::ResultErrors{WrappedExceptionMemoriaError::create(std::current_exception())};
    }
}


template <typename Fn>
std::enable_if_t<
    detail::IsVoidResultH<std::invoke_result_t<Fn>>::Value,
    void
>
wrap_construction(MaybeError& maybe_error, Fn&& fn) {
    if (!maybe_error)
    {
        try {
            fn();
        }
        catch (...) {
            maybe_error = detail::ResultErrors{WrappedExceptionMemoriaError::create(std::current_exception())};
        }
    }
}

template <typename Fn>
std::enable_if_t<
    !detail::IsVoidResultH<std::invoke_result_t<Fn>>::Value,
    void
>
wrap_construction(MaybeError& maybe_error, Fn&& fn) {
    if (!maybe_error)
    {
        try {
            auto res = fn();
            if (isFail(res)){
                maybe_error = detail::propagate_errors(std::move(res));
            }
        }
        catch (...) {
            maybe_error = detail::ResultErrors{WrappedExceptionMemoriaError::create(std::current_exception())};
        }
    }
}

#define MEMORIA_RETURN_IF_ERROR(ResultVal)   \
do {                                         \
    if (MMA_UNLIKELY(ResultVal.is_error())) {  \
        return std::move(ResultVal).transfer_error(); \
    }                                        \
} while (0)

#define MEMORIA_TRY_VOID(FnCall)             \
do {                                         \
    auto res0 = FnCall;                      \
    if (MMA_UNLIKELY(res0.is_error())) {       \
        return std::move(res0).transfer_error(); \
    }                                        \
} while(0)

#define MEMORIA_TRY(VarName, Code)                  \
    auto VarName##_result = Code;                  \
    if (MMA_UNLIKELY(VarName##_result.is_error())) return std::move(VarName##_result).transfer_error(); \
    auto& VarName = VarName##_result.get()

#define MEMORIA_PROPAGATE_ERROR(res0) std::move(res0).transfer_error()

}

namespace fmt {

template <typename T>
struct formatter<memoria::Result<T>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::Result<T>& d, FormatContext& ctx) {
        std::stringstream buf;
        buf << d;
        return format_to(ctx.out(), "{}", buf.str());
    }
};

}

