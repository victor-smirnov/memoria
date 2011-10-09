
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_EXCEPTIONS_FILE_HPP
#define	_MEMORIA_VAPI_EXCEPTIONS_FILE_HPP

#include <memoria/core/exceptions/memoria.hpp>
#include <string>

namespace memoria    {
namespace vapi       {

using namespace std;

class MEMORIA_API FileException: public MemoriaException {
	String file_name_;

public:
    FileException(StringRef source, StringRef message, StringRef file_name):
                MemoriaException(source, message), file_name_(file_name) {}

    StringRef FileName() const {
        return file_name_;
    }
};


}
}
#endif
