
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_COLLECTIONS_MANAGER_HPP
#define	_MEMORIA_VAPI_COLLECTIONS_MANAGER_HPP

#include <memoria/metadata/metadata.hpp>

#include <memoria/vapi/models/logs.hpp>

#include <fstream>

namespace memoria    {
namespace vapi       {

struct MEMORIA_API Allocator {
	enum {NONE = 0, ROOT = 1};
};

struct MEMORIA_API InputStreamHandler {
    virtual Int available() 							= 0;
    virtual void close() 								= 0;
    virtual Int buffer_size() 							= 0;
    virtual BigInt pos() 								= 0;
    virtual Int read(void* mem, Int offset, Int length) = 0;
    virtual bool read(void* mem, Int size)				= 0;


    template <typename T>
    bool read(T &value) {
        return read(&value, sizeof(T));
    }
};



struct MEMORIA_API OutputStreamHandler {

    virtual Int buffer_size() = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual BigInt pos() = 0;
    virtual void write(const void* mem, int offset, int lenght) = 0;

    template <typename T>
    void write(const T& value) {
        write(&value, 0, sizeof(T));
    }
};

class MEMORIA_API FileOutputStreamHandler: public OutputStreamHandler {
public:
	static FileOutputStreamHandler* create(const char* file);
};

class MEMORIA_API FileInputStreamHandler: public InputStreamHandler {
public:
	static FileInputStreamHandler* create(const char* file);
};

}
}

#endif
