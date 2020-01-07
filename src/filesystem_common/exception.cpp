//  boost/filesystem/exception.hpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2003
//  Copyright Andrey Semashev 2019

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#include <string>
#include <memoria/v1/filesystem/config.hpp>
#include <memoria/v1/filesystem/path.hpp>
#include <memoria/v1/filesystem/exception.hpp>

#include "../filesystem_common/error_handling.hpp"

#include <boost/config/abi_prefix.hpp> // must be the last #include

namespace memoria { namespace v1 {
namespace filesystem {

filesystem_error::filesystem_error(const std::string& what_arg, boost::system::error_code ec) :
  boost::system::system_error(ec, what_arg)
{
  try
  {
    m_imp_ptr.reset(new impl());
  }
  catch (...)
  {
    m_imp_ptr.reset();
  }
}

filesystem_error::filesystem_error(const std::string& what_arg, const path& path1_arg, boost::system::error_code ec) :
  boost::system::system_error(ec, what_arg)
{
  try
  {
    m_imp_ptr.reset(new impl(path1_arg));
  }
  catch (...)
  {
    m_imp_ptr.reset();
  }
}

filesystem_error::filesystem_error(const std::string& what_arg, const path& path1_arg, const path& path2_arg, boost::system::error_code ec) :
  boost::system::system_error(ec, what_arg)
{
  try
  {
    m_imp_ptr.reset(new impl(path1_arg, path2_arg));
  }
  catch (...)
  {
    m_imp_ptr.reset();
  }
}

filesystem_error::filesystem_error(filesystem_error const& that) :
  boost::system::system_error(static_cast< boost::system::system_error const& >(that)),
  m_imp_ptr(that.m_imp_ptr)
{
}

filesystem_error& filesystem_error::operator= (filesystem_error const& that)
{
  static_cast< boost::system::system_error& >(*this) = static_cast< boost::system::system_error const& >(that);
  m_imp_ptr = that.m_imp_ptr;
  return *this;
}

filesystem_error::~filesystem_error() BOOST_NOEXCEPT_OR_NOTHROW
{
}

const char* filesystem_error::what() const BOOST_NOEXCEPT_OR_NOTHROW
{
  if (m_imp_ptr.get()) try
  {
    if (m_imp_ptr->m_what.empty())
    {
      m_imp_ptr->m_what = boost::system::system_error::what();
      if (!m_imp_ptr->m_path1.empty())
      {
        m_imp_ptr->m_what += ": \"";
        m_imp_ptr->m_what += m_imp_ptr->m_path1.string();
        m_imp_ptr->m_what += "\"";
      }
      if (!m_imp_ptr->m_path2.empty())
      {
        m_imp_ptr->m_what += ", \"";
        m_imp_ptr->m_what += m_imp_ptr->m_path2.string();
        m_imp_ptr->m_what += "\"";
      }
    }

    return m_imp_ptr->m_what.c_str();
  }
  catch (...)
  {
    m_imp_ptr->m_what.clear();
  }

  return boost::system::system_error::what();
}

const path& filesystem_error::get_empty_path() BOOST_NOEXCEPT
{
  static const path empty_path;
  return empty_path;
}

//  error handling helpers declared in error_handling.hpp  -----------------------------------------------------//

void emit_error(err_t error_num, boost::system::error_code* ec, const char* message)
{
  if (!ec)
    MEMORIA_BOOST_FILESYSTEM_THROW(filesystem_error(message, boost::system::error_code(error_num, boost::system::system_category())));
  else
    ec->assign(error_num, boost::system::system_category());
}

void emit_error(err_t error_num, const path& p, boost::system::error_code* ec, const char* message)
{
  if (!ec)
    MEMORIA_BOOST_FILESYSTEM_THROW(filesystem_error(message, p, boost::system::error_code(error_num, boost::system::system_category())));
  else
    ec->assign(error_num, boost::system::system_category());
}

void emit_error(err_t error_num, const path& p1, const path& p2, boost::system::error_code* ec, const char* message)
{
  if (ec == 0)
    MEMORIA_BOOST_FILESYSTEM_THROW(filesystem_error(message, p1, p2, boost::system::error_code(error_num, boost::system::system_category())));
  else
    ec->assign(error_num, boost::system::system_category());
}

} // namespace filesystem
}} // namespace memoria::v1

#include <boost/config/abi_suffix.hpp> // pops abi_prefix.hpp pragmas
