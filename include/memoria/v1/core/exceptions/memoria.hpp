
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>
#include <memoria/v1/core/tools/strings/string_buffer.hpp>
#include <string>

#include <stdlib.h>

namespace memoria {
namespace v1 {

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


class OOMException: public MemoriaThrowable {

public:
    OOMException(const char* source): MemoriaThrowable(source) {}
};






const char* ExtractMemoriaPath(const char* path);

ostream& operator<<(ostream& out, const MemoriaThrowable& t);

}}
