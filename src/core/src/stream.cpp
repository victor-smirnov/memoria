
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/core/tools/stream.hpp>
#include <memoria/core/tools/strings.hpp>


#ifdef __GNUC__
#include <unistd.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>



namespace memoria { namespace vapi {

using namespace std;




FileOutputStreamHandler* FileOutputStreamHandler::create(const char* file) {
	return new FileOutputStreamHandlerImpl(file);
}

FileInputStreamHandler* FileInputStreamHandler::create(const char* file) {
	return new FileInputStreamHandlerImpl(file);
}



}}


