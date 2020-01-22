//  boost/filesystem/operations.hpp  ---------------------------------------------------//

//  Copyright Beman Dawes 2002-2009
//  Copyright Jan Langer 2002
//  Copyright Dietmar Kuehl 2001
//  Copyright Vladimir Prus 2002

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#ifndef MEMORIA_BOOST_FILESYSTEM3_OPERATIONS_HPP
#define MEMORIA_BOOST_FILESYSTEM3_OPERATIONS_HPP

#include <boost/config.hpp>

# if defined( BOOST_NO_STD_WSTRING )
#   error Configuration not supported: Boost.Filesystem V3 and later requires std::wstring support
# endif

#include <memoria/filesystem/config.hpp>
#include <memoria/filesystem/path.hpp>
#include <memoria/filesystem/file_status.hpp>

#ifndef MEMORIA_BOOST_FILESYSTEM_NO_DEPRECATED
// These includes are left for backward compatibility and should be included directly by users, as needed
#include <memoria/filesystem/exception.hpp>
#include <memoria/filesystem/directory.hpp>
#endif

#include <boost/core/scoped_enum.hpp>
#include <boost/system/error_code.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <ctime>

#include <boost/config/abi_prefix.hpp> // must be the last #include

//--------------------------------------------------------------------------------------//

namespace memoria {
namespace filesystem {

struct space_info
{
  // all values are byte counts
  boost::uintmax_t capacity;
  boost::uintmax_t free;      // <= capacity
  boost::uintmax_t available; // <= free
};

BOOST_SCOPED_ENUM_DECLARE_BEGIN(copy_option)
  {none=0, fail_if_exists = none, overwrite_if_exists}
BOOST_SCOPED_ENUM_DECLARE_END(copy_option)

//--------------------------------------------------------------------------------------//
//                             implementation details                                   //
//--------------------------------------------------------------------------------------//

namespace detail {

//  We cannot pass a BOOST_SCOPED_ENUM to a compled function because it will result
//  in an undefined reference if the library is compled with -std=c++0x but the use
//  is compiled in C++03 mode, or vice versa. See tickets 6124, 6779, 10038.
enum copy_option {none=0, fail_if_exists = none, overwrite_if_exists};

MEMORIA_BOOST_FILESYSTEM_DECL
file_status status(const path&p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
file_status symlink_status(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
bool is_empty(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path initial_path(boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path canonical(const path& p, const path& base, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void copy(const path& from, const path& to, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void copy_directory(const path& from, const path& to, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void copy_file(const path& from, const path& to,  // See ticket #2925
                detail::copy_option option, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void copy_symlink(const path& existing_symlink, const path& new_symlink, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
bool create_directories(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
bool create_directory(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void create_directory_symlink(const path& to, const path& from,
                              boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void create_hard_link(const path& to, const path& from, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void create_symlink(const path& to, const path& from, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path current_path(boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void current_path(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
bool equivalent(const path& p1, const path& p2, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
boost::uintmax_t file_size(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
boost::uintmax_t hard_link_count(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
std::time_t last_write_time(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void last_write_time(const path& p, const std::time_t new_time,
                     boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void permissions(const path& p, perms prms, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path read_symlink(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path relative(const path& p, const path& base, boost::system::error_code* ec = 0);
MEMORIA_BOOST_FILESYSTEM_DECL
bool remove(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
boost::uintmax_t remove_all(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void rename(const path& old_p, const path& new_p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
void resize_file(const path& p, uintmax_t size, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
space_info space(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path system_complete(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path temp_directory_path(boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path unique_path(const path& p, boost::system::error_code* ec=0);
MEMORIA_BOOST_FILESYSTEM_DECL
path weakly_canonical(const path& p, boost::system::error_code* ec = 0);

} // namespace detail

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                             status query functions                                   //
//                                                                                      //
//--------------------------------------------------------------------------------------//

inline
file_status status(const path& p)    {return detail::status(p);}
inline
file_status status(const path& p, boost::system::error_code& ec)
                                     {return detail::status(p, &ec);}
inline
file_status symlink_status(const path& p) {return detail::symlink_status(p);}
inline
file_status symlink_status(const path& p, boost::system::error_code& ec)
                                     {return detail::symlink_status(p, &ec);}
inline
bool exists(const path& p)           {return exists(detail::status(p));}
inline
bool exists(const path& p, boost::system::error_code& ec)
                                     {return exists(detail::status(p, &ec));}
inline
bool is_directory(const path& p)     {return is_directory(detail::status(p));}
inline
bool is_directory(const path& p, boost::system::error_code& ec)
                                     {return is_directory(detail::status(p, &ec));}
inline
bool is_regular_file(const path& p)  {return is_regular_file(detail::status(p));}
inline
bool is_regular_file(const path& p, boost::system::error_code& ec)
                                     {return is_regular_file(detail::status(p, &ec));}
inline
bool is_other(const path& p)         {return is_other(detail::status(p));}
inline
bool is_other(const path& p, boost::system::error_code& ec)
                                     {return is_other(detail::status(p, &ec));}
inline
bool is_symlink(const path& p)       {return is_symlink(detail::symlink_status(p));}
inline
bool is_symlink(const path& p, boost::system::error_code& ec)
                                     {return is_symlink(detail::symlink_status(p, &ec));}
#ifndef MEMORIA_BOOST_FILESYSTEM_NO_DEPRECATED
inline
bool is_regular(const path& p)       {return is_regular(detail::status(p));}
inline
bool is_regular(const path& p, boost::system::error_code& ec)
                                     {return is_regular(detail::status(p, &ec));}
#endif

inline
bool is_empty(const path& p)         {return detail::is_empty(p);}
inline
bool is_empty(const path& p, boost::system::error_code& ec)
                                     {return detail::is_empty(p, &ec);}

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                             operational functions                                    //
//                  in alphabetical order, unless otherwise noted                       //
//                                                                                      //
//--------------------------------------------------------------------------------------//

//  forward declarations
path current_path();  // fwd declaration
path initial_path();

MEMORIA_BOOST_FILESYSTEM_DECL
path absolute(const path& p, const path& base=current_path());
//  If base.is_absolute(), throws nothing. Thus no need for ec argument

inline
path canonical(const path& p, const path& base=current_path())
                                     {return detail::canonical(p, base);}
inline
path canonical(const path& p, boost::system::error_code& ec)
                                     {return detail::canonical(p, current_path(), &ec);}
inline
path canonical(const path& p, const path& base, boost::system::error_code& ec)
                                     {return detail::canonical(p, base, &ec);}

#ifndef MEMORIA_BOOST_FILESYSTEM_NO_DEPRECATED
inline
path complete(const path& p)
{
  return absolute(p, initial_path());
}

inline
path complete(const path& p, const path& base)
{
  return absolute(p, base);
}
#endif

inline
void copy(const path& from, const path& to) {detail::copy(from, to);}

inline
void copy(const path& from, const path& to, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::copy(from, to, &ec);}
inline
void copy_directory(const path& from, const path& to)
                                     {detail::copy_directory(from, to);}
inline
void copy_directory(const path& from, const path& to, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::copy_directory(from, to, &ec);}
inline
void copy_file(const path& from, const path& to,   // See ticket #2925
               BOOST_SCOPED_ENUM_NATIVE(copy_option) option)
{
  detail::copy_file(from, to, static_cast<detail::copy_option>(option));
}
inline
void copy_file(const path& from, const path& to)
{
  detail::copy_file(from, to, detail::fail_if_exists);
}
inline
void copy_file(const path& from, const path& to,   // See ticket #2925
               BOOST_SCOPED_ENUM_NATIVE(copy_option) option, boost::system::error_code& ec) BOOST_NOEXCEPT
{
  detail::copy_file(from, to, static_cast<detail::copy_option>(option), &ec);
}
inline
void copy_file(const path& from, const path& to, boost::system::error_code& ec) BOOST_NOEXCEPT
{
  detail::copy_file(from, to, detail::fail_if_exists, &ec);
}
inline
void copy_symlink(const path& existing_symlink,
                  const path& new_symlink) {detail::copy_symlink(existing_symlink, new_symlink);}

inline
void copy_symlink(const path& existing_symlink, const path& new_symlink,
                  boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::copy_symlink(existing_symlink, new_symlink, &ec);}
inline
bool create_directories(const path& p) {return detail::create_directories(p);}

inline
bool create_directories(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::create_directories(p, &ec);}
inline
bool create_directory(const path& p) {return detail::create_directory(p);}

inline
bool create_directory(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::create_directory(p, &ec);}
inline
void create_directory_symlink(const path& to, const path& from)
                                     {detail::create_directory_symlink(to, from);}
inline
void create_directory_symlink(const path& to, const path& from, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::create_directory_symlink(to, from, &ec);}
inline
void create_hard_link(const path& to, const path& new_hard_link) {detail::create_hard_link(to, new_hard_link);}

inline
void create_hard_link(const path& to, const path& new_hard_link, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::create_hard_link(to, new_hard_link, &ec);}
inline
void create_symlink(const path& to, const path& new_symlink) {detail::create_symlink(to, new_symlink);}

inline
void create_symlink(const path& to, const path& new_symlink, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::create_symlink(to, new_symlink, &ec);}
inline
path current_path()                  {return detail::current_path();}

inline
path current_path(boost::system::error_code& ec) {return detail::current_path(&ec);}

inline
void current_path(const path& p)     {detail::current_path(p);}

inline
void current_path(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT {detail::current_path(p, &ec);}

inline
bool equivalent(const path& p1, const path& p2) {return detail::equivalent(p1, p2);}

inline
bool equivalent(const path& p1, const path& p2, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::equivalent(p1, p2, &ec);}
inline
boost::uintmax_t file_size(const path& p) {return detail::file_size(p);}

inline
boost::uintmax_t file_size(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::file_size(p, &ec);}
inline
boost::uintmax_t hard_link_count(const path& p) {return detail::hard_link_count(p);}

inline
boost::uintmax_t hard_link_count(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::hard_link_count(p, &ec);}
inline
path initial_path()                  {return detail::initial_path();}

inline
path initial_path(boost::system::error_code& ec) {return detail::initial_path(&ec);}

template <class Path>
path initial_path() {return initial_path();}
template <class Path>
path initial_path(boost::system::error_code& ec) {return detail::initial_path(&ec);}

inline
std::time_t last_write_time(const path& p) {return detail::last_write_time(p);}

inline
std::time_t last_write_time(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::last_write_time(p, &ec);}
inline
void last_write_time(const path& p, const std::time_t new_time)
                                     {detail::last_write_time(p, new_time);}
inline
void last_write_time(const path& p, const std::time_t new_time,
                     boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::last_write_time(p, new_time, &ec);}
inline
void permissions(const path& p, perms prms)
                                     {detail::permissions(p, prms);}
inline
void permissions(const path& p, perms prms, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::permissions(p, prms, &ec);}

inline
path read_symlink(const path& p)     {return detail::read_symlink(p);}

inline
path read_symlink(const path& p, boost::system::error_code& ec)
                                     {return detail::read_symlink(p, &ec);}

inline
bool remove(const path& p)           {return detail::remove(p);}

inline
bool remove(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::remove(p, &ec);}

inline
boost::uintmax_t remove_all(const path& p) {return detail::remove_all(p);}

inline
boost::uintmax_t remove_all(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::remove_all(p, &ec);}
inline
void rename(const path& old_p, const path& new_p) {detail::rename(old_p, new_p);}

inline
void rename(const path& old_p, const path& new_p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::rename(old_p, new_p, &ec);}
inline  // name suggested by Scott McMurray
void resize_file(const path& p, uintmax_t size) {detail::resize_file(p, size);}

inline
void resize_file(const path& p, uintmax_t size, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {detail::resize_file(p, size, &ec);}
inline
path relative(const path& p, const path& base=current_path())
                                     {return detail::relative(p, base);}
inline
path relative(const path& p, boost::system::error_code& ec)
                                     {return detail::relative(p, current_path(), &ec);}
inline
path relative(const path& p, const path& base, boost::system::error_code& ec)
                                     {return detail::relative(p, base, &ec);}
inline
space_info space(const path& p)      {return detail::space(p);}

inline
space_info space(const path& p, boost::system::error_code& ec) BOOST_NOEXCEPT
                                     {return detail::space(p, &ec);}

#ifndef MEMORIA_BOOST_FILESYSTEM_NO_DEPRECATED
inline bool symbolic_link_exists(const path& p)
                                     { return is_symlink(memoria::filesystem::symlink_status(p)); }
#endif

inline
path system_complete(const path& p)  {return detail::system_complete(p);}

inline
path system_complete(const path& p, boost::system::error_code& ec)
                                     {return detail::system_complete(p, &ec);}
inline
path temp_directory_path()           {return detail::temp_directory_path();}

inline
path temp_directory_path(boost::system::error_code& ec)
                                     {return detail::temp_directory_path(&ec);}
inline
path unique_path(const path& p="%%%%-%%%%-%%%%-%%%%")
                                     {return detail::unique_path(p);}
inline
path unique_path(const path& p, boost::system::error_code& ec)
                                     {return detail::unique_path(p, &ec);}
inline
path weakly_canonical(const path& p)   {return detail::weakly_canonical(p);}

inline
path weakly_canonical(const path& p, boost::system::error_code& ec)
                                     {return detail::weakly_canonical(p, &ec);}

//  test helper  -----------------------------------------------------------------------//

//  Not part of the documented interface since false positives are possible;
//  there is no law that says that an OS that has large stat.st_size
//  actually supports large file sizes.

namespace detail {

MEMORIA_BOOST_FILESYSTEM_DECL bool possible_large_file_size_support();

} // namespace detail

} // namespace filesystem
} // namespace memoria

#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#endif // MEMORIA_BOOST_FILESYSTEM3_OPERATIONS_HPP