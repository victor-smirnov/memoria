/*
 A C++ interface to POSIX functions.

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#pragma once

#if defined(__MINGW32__) || defined(__CYGWIN__)
// Workaround MinGW bug https://sourceforge.net/p/mingw/bugs/2024/.
# undef __STRICT_ANSI__
#endif

#include <errno.h>
#include <fcntl.h>   // for O_RDONLY
#include <locale.h>  // for locale_t
#include <stdio.h>
#include <stdlib.h>  // for strtod_l

#include <cstddef>

#if defined __APPLE__ || defined(__FreeBSD__)
# include <xlocale.h>  // for LC_NUMERIC_MASK on OS X
#endif

#include <memoria/v1/fmt/format.hpp>

#ifndef MMA1_FMT_POSIX
# if defined(_WIN32) && !defined(__MINGW32__)
// Fix warnings about deprecated symbols.
#  define MMA1_FMT_POSIX(call) _##call
# else
#  define MMA1_FMT_POSIX(call) call
# endif
#endif

// Calls to system functions are wrapped in MMA1_FMT_SYSTEM for testability.
#ifdef MMA1_FMT_SYSTEM
# define MMA1_FMT_POSIX_CALL(call) MMA1_FMT_SYSTEM(call)
#else
# define MMA1_FMT_SYSTEM(call) call
# ifdef _WIN32
// Fix warnings about deprecated symbols.
#  define MMA1_FMT_POSIX_CALL(call) ::_##call
# else
#  define MMA1_FMT_POSIX_CALL(call) ::call
# endif
#endif

// Retries the expression while it evaluates to error_result and errno
// equals to EINTR.
#ifndef _WIN32
# define MMA1_FMT_RETRY_VAL(result, expression, error_result) \
  do { \
    result = (expression); \
  } while (result == error_result && errno == EINTR)
#else
# define MMA1_FMT_RETRY_VAL(result, expression, error_result) result = (expression)
#endif

#define MMA1_FMT_RETRY(result, expression) MMA1_FMT_RETRY_VAL(result, expression, -1)

namespace memoria {
namespace v1 {
namespace fmt {

// An error code.
class ErrorCode {
 private:
  int value_;

 public:
  explicit ErrorCode(int value = 0) noexcept : value_(value) {}

  int get() const noexcept { return value_; }
};

// A buffered file.
class BufferedFile {
 private:
  FILE *file_;

  friend class File;

  explicit BufferedFile(FILE *f) : file_(f) {}

 public:
  // Constructs a BufferedFile object which doesn't represent any file.
  BufferedFile() noexcept : file_(nullptr) {}

  // Destroys the object closing the file it represents if any.
  MMA1_FMT_API ~BufferedFile() noexcept;


 private:
  MMA1_FMT_DISALLOW_COPY_AND_ASSIGN(BufferedFile);

 public:
  BufferedFile(BufferedFile &&other) noexcept : file_(other.file_) {
    other.file_ = nullptr;
  }

  BufferedFile& operator=(BufferedFile &&other) {
    close();
    file_ = other.file_;
    other.file_ = nullptr;
    return *this;
  }

  // Opens a file.
  MMA1_FMT_API BufferedFile(CStringRef filename, CStringRef mode);

  // Closes the file.
  MMA1_FMT_API void close();

  // Returns the pointer to a FILE object representing this file.
  FILE *get() const noexcept { return file_; }

  // We place parentheses around fileno to workaround a bug in some versions
  // of MinGW that define fileno as a macro.
  MMA1_FMT_API int (fileno)() const;

  void print(CStringRef format_str, const ArgList &args) {
    fmt::print(file_, format_str, args);
  }
  MMA1_FMT_VARIADIC(void, print, CStringRef)
};

// A file. Closed file is represented by a File object with descriptor -1.
// Methods that are not declared with MMA1_FMT_NOEXCEPT may throw
// fmt::SystemError in case of failure. Note that some errors such as
// closing the file multiple times will cause a crash on Windows rather
// than an exception. You can get standard behavior by overriding the
// invalid parameter handler with _set_invalid_parameter_handler.
class File {
 private:
  int fd_;  // File descriptor.

  // Constructs a File object with a given descriptor.
  explicit File(int fd) : fd_(fd) {}

 public:
  // Possible values for the oflag argument to the constructor.
  enum {
    RDONLY = MMA1_FMT_POSIX(O_RDONLY), // Open for reading only.
    WRONLY = MMA1_FMT_POSIX(O_WRONLY), // Open for writing only.
    RDWR   = MMA1_FMT_POSIX(O_RDWR)    // Open for reading and writing.
  };

  // Constructs a File object which doesn't represent any file.
  File() noexcept : fd_(-1) {}

  // Opens a file and constructs a File object representing this file.
  MMA1_FMT_API File(CStringRef path, int oflag);


 private:
  MMA1_FMT_DISALLOW_COPY_AND_ASSIGN(File);

 public:
  File(File &&other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
  }

  File& operator=(File &&other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }


  // Destroys the object closing the file it represents if any.
  MMA1_FMT_API ~File() noexcept;

  // Returns the file descriptor.
  int descriptor() const noexcept { return fd_; }

  // Closes the file.
  MMA1_FMT_API void close();

  // Returns the file size. The size has signed type for consistency with
  // stat::st_size.
  MMA1_FMT_API LongLong size() const;

  // Attempts to read count bytes from the file into the specified buffer.
  MMA1_FMT_API std::size_t read(void *buffer, std::size_t count);

  // Attempts to write count bytes from the specified buffer to the file.
  MMA1_FMT_API std::size_t write(const void *buffer, std::size_t count);

  // Duplicates a file descriptor with the dup function and returns
  // the duplicate as a file object.
  MMA1_FMT_API static File dup(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  MMA1_FMT_API void dup2(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  MMA1_FMT_API void dup2(int fd, ErrorCode &ec) noexcept;

  // Creates a pipe setting up read_end and write_end file objects for reading
  // and writing respectively.
  MMA1_FMT_API static void pipe(File &read_end, File &write_end);

  // Creates a BufferedFile object associated with this file and detaches
  // this File object from the file.
  MMA1_FMT_API BufferedFile fdopen(const char *mode);
};

// Returns the memory page size.
long getpagesize();

#if (defined(LC_NUMERIC_MASK) || defined(_MSC_VER)) && \
    !defined(__ANDROID__) && !defined(__CYGWIN__)
# define MMA1_FMT_LOCALE
#endif

#ifdef MMA1_FMT_LOCALE
// A "C" numeric locale.
class Locale {
 private:
# ifdef _MSC_VER
  typedef _locale_t locale_t;

  enum { LC_NUMERIC_MASK = LC_NUMERIC };

  static locale_t newlocale(int category_mask, const char *locale, locale_t) {
    return _create_locale(category_mask, locale);
  }

  static void freelocale(locale_t locale) {
    _free_locale(locale);
  }

  static double strtod_l(const char *nptr, char **endptr, _locale_t locale) {
    return _strtod_l(nptr, endptr, locale);
  }
# endif

  locale_t locale_;

  MMA1_FMT_DISALLOW_COPY_AND_ASSIGN(Locale);

 public:
  typedef locale_t Type;

  Locale() : locale_(newlocale(LC_NUMERIC_MASK, "C", nullptr)) {
    if (!locale_)
      throw fmt::SystemError(errno, "cannot create locale");
  }
  ~Locale() { freelocale(locale_); }

  Type get() const { return locale_; }

  // Converts string to floating-point number and advances str past the end
  // of the parsed input.
  double strtod(const char *&str) const {
    char *end = nullptr;
    double result = strtod_l(str, &end, locale_);
    str = end;
    return result;
  }
};
#endif  // MMA1_FMT_LOCALE
}  // namespace fmt
}}




