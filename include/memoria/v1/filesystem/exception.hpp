//  boost/filesystem/exception.hpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2003
//  Copyright Andrey Semashev 2019

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#ifndef MEMORIA_BOOST_FILESYSTEM3_EXCEPTION_HPP
#define MEMORIA_BOOST_FILESYSTEM3_EXCEPTION_HPP

#include <boost/config.hpp>

# if defined( BOOST_NO_STD_WSTRING )
#   error Configuration not supported: Boost.Filesystem V3 and later requires std::wstring support
# endif

#include <memoria/v1/filesystem/config.hpp>
#include <memoria/v1/filesystem/path.hpp>

#include <string>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <boost/config/abi_prefix.hpp> // must be the last #include

#if defined(BOOST_MSVC)
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif

namespace memoria { namespace v1 {
namespace filesystem {

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                            class filesystem_error                                    //
//                                                                                      //
//--------------------------------------------------------------------------------------//

class MEMORIA_BOOST_FILESYSTEM_DECL filesystem_error :
  public boost::system::system_error
{
  // see http://www.boost.org/more/error_handling.html for design rationale

public:
  filesystem_error(const std::string& what_arg, boost::system::error_code ec);
  filesystem_error(const std::string& what_arg, const path& path1_arg, boost::system::error_code ec);
  filesystem_error(const std::string& what_arg, const path& path1_arg, const path& path2_arg, boost::system::error_code ec);

  filesystem_error(filesystem_error const& that);
  filesystem_error& operator= (filesystem_error const& that);

  ~filesystem_error() noexcept;

  const path& path1() const BOOST_NOEXCEPT
  {
    return m_imp_ptr.get() ? m_imp_ptr->m_path1 : get_empty_path();
  }
  const path& path2() const BOOST_NOEXCEPT
  {
    return m_imp_ptr.get() ? m_imp_ptr->m_path2 : get_empty_path();
  }

  const char* what() const noexcept;

private:
  static const path& get_empty_path() BOOST_NOEXCEPT;

private:
  struct impl :
    public boost::intrusive_ref_counter< impl >
  {
    path         m_path1; // may be empty()
    path         m_path2; // may be empty()
    std::string  m_what;  // not built until needed

    BOOST_DEFAULTED_FUNCTION(impl(), {})
    explicit impl(path const& path1) : m_path1(path1) {}
    impl(path const& path1, path const& path2) : m_path1(path1), m_path2(path2) {}
  };
  ::boost::intrusive_ptr< impl > m_imp_ptr;
};

} // namespace filesystem
}} // namespace memoria::v1

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
#endif // MEMORIA_BOOST_FILESYSTEM3_EXCEPTION_HPP
