
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_EXCEPTIONS_MEMORIA_HPP
#define _MEMORIA_VAPI_EXCEPTIONS_MEMORIA_HPP

#include <string>
#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/strings/string_buffer.hpp>



#include <stdlib.h>

namespace memoria    {
namespace vapi       {

using namespace std;

class MemoriaThrowable {
protected:
    const char* source_;
public:
    MemoriaThrowable(const char* source): source_(source) {}

    const char* source() const {
        return source_;
    }

    virtual void dump(ostream& out) const {}
};


class Exception: public MemoriaThrowable {

    String message_;
public:
    Exception(const char* source, StringRef message): MemoriaThrowable(source), message_(message)           {}
    Exception(const char* source, const SBuf& message): MemoriaThrowable(source), message_(message.str())   {}

    StringRef message() const {
        return message_;
    }

    virtual void dump(ostream& out) const {
        out<<message_;
    }
};

class CtrTypeException: public Exception {

public:
    CtrTypeException(const char* source, StringRef message): Exception(source, message)     {}
    CtrTypeException(const char* source, const SBuf& message): Exception(source, message)   {}
};

class NoCtrException: public Exception {

public:
    NoCtrException(const char* source, StringRef message): Exception(source, message)     {}
    NoCtrException(const char* source, const SBuf& message): Exception(source, message)   {}
};

class CtrAlreadyExistsException: public Exception {

public:
    CtrAlreadyExistsException(const char* source, StringRef message): Exception(source, message)     {}
    CtrAlreadyExistsException(const char* source, const SBuf& message): Exception(source, message)   {}
};


class RollbackException: public Exception {
public:
    RollbackException(const char* source, StringRef message): Exception(source, message) {}
};


class MEMORIA_API OOMException: public MemoriaThrowable {

public:
    OOMException(const char* source): MemoriaThrowable(source) {}
};


class MEMORIA_API BoundsException: public ::memoria::vapi::Exception {

public:
    BoundsException(const char* source, StringRef message):
                Exception(source, message) {}

    BoundsException(const char* source, const SBuf& message):
                Exception(source, message) {}
};




MEMORIA_API const char* ExtractMemoriaPath(const char* path);

ostream& operator<<(ostream& out, const MemoriaThrowable& t);

}
}

#endif
