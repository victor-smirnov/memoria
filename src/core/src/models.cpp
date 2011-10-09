
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/core/vapi/api.hpp>



namespace memoria { namespace vapi {

LogHandler* Logger::default_handler_ = new DefaultLogHandlerImpl();
Logger logger("Memoria", Logger::INFO, NULL);




}}


