
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

class MEMORIA_API MemoriaException {
    BigInt data_;
    string source_;
    string message_;
public:
    MemoriaException(const string &source, const string &message): data_(0), source_(source), message_(message) 	{}
    MemoriaException(const string &source, const SBuf &message): data_(0), source_(source), message_(message.Str())	{}


    MemoriaException(const string &source, const string &message, BigInt data): data_(data), source_(source), message_(message) {}

    BigInt data() const {
        return data_;
    }

    const string &source() const {
        return source_;
    }

    const string &message() const {
        return message_;
    }
};

class MEMORIA_API MemoriaSigSegv {
    BigInt data_;
    string source_;
    string message_;
public:
    MemoriaSigSegv(const string &source, const string &message): data_(0), source_(source), message_(message){
    	//abort();
    }

    MemoriaSigSegv(const string &source, const string &message, BigInt data): data_(data), source_(source), message_(message) {
    	//abort();
    }

    BigInt data() const {
        return data_;
    }

    const string &source() const {
        return source_;
    }

    const string &message() const {
        return message_;
    }
};


MEMORIA_API const char* ExtractMemoriaPath(const char* path);

}
}
#endif
