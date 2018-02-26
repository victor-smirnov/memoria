// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/v1/process/detail/config.hpp>
#include <memoria/v1/process/detail/handler_base.hpp>
#include <memoria/v1/process/detail/windows/async_handler.hpp>
#include <system_error>
#include <functional>

namespace boost { namespace process { namespace detail { namespace windows {

struct on_exit_ : boost::process::detail::windows::async_handler
{
    std::function<void(int, const std::error_code&)> handler;
    on_exit_(const std::function<void(int, const std::error_code&)> & handler) : handler(handler)
    {

    }

    template<typename Executor>
    std::function<void(int, const std::error_code&)> on_exit_handler(Executor&)
    {
        auto handler = this->handler;
        return [handler](int exit_code, const std::error_code & ec)
               {
                    handler(static_cast<int>(exit_code), ec);
               };

    }
};


}}}}

