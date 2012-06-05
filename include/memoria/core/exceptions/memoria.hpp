
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_EXCEPTIONS_MEMORIA_HPP
#define	_MEMORIA_VAPI_EXCEPTIONS_MEMORIA_HPP

#include <string>
#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/string_buffer.hpp>
#include <stdlib.h>

namespace memoria    {
namespace vapi       {

using namespace std;

class MemoriaThrowable {
	const char* source_;
public:
	MemoriaThrowable(const char* source): source_(source) {}

	const char* source() const {
		return source_;
	}

	virtual void Out(ostream& out) const {}
};


class MEMORIA_API Exception: public MemoriaThrowable {

    String message_;
public:
    Exception(const char* source, StringRef message): MemoriaThrowable(source), message_(message) 			{}
    Exception(const char* source, const SBuf& message): MemoriaThrowable(source), message_(message.Str())	{}


    StringRef message() const {
        return message_;
    }

    virtual void Out(ostream& out) const {
    	out<<message_;
    }
};

class MemoriaSigSegv: public Exception {
public:
    MemoriaSigSegv(const char* source, StringRef message): Exception(source, message) {}
};


MEMORIA_API const char* ExtractMemoriaPath(const char* path);

}
}

namespace std {
using namespace memoria::vapi;

ostream& operator<<(ostream& out, MemoriaThrowable& t);

}

#endif
