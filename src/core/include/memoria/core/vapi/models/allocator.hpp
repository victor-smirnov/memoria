
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_COLLECTIONS_MANAGER_HPP
#define	_MEMORIA_CORE_API_COLLECTIONS_MANAGER_HPP

#include <memoria/vapi/models/allocator.hpp>
#include <memoria/core/vapi/metadata/page.hpp>
#include <memoria/vapi/models/logs.hpp>



//#define MEMORIA_THROW_NOT_SUPPORTED {throw MemoriaException(MEMORIA_SOURCE, "Method is not supported for this Container type");}

namespace memoria    {
namespace vapi       {


template <typename Interface>
class InputStreamHandlerImplT: public Interface {
	typedef InputStreamHandlerImplT<Interface>	Me;
	typedef Interface 							Base;
public:

    virtual Int available() = 0;
    virtual void close() = 0;
    virtual Int buffer_size() = 0;
    virtual Int read(void* mem, Int offset, Int length) = 0;

    virtual bool read(void* mem, Int size)
    {
        Int size0 = size;
        Int ptr = 0;
        while (size > 0) {
            Int r = read(mem, ptr, size);
            if (r < 0) {
                if (size != size0) {
                    throw MemoriaException(MEMORIA_SOURCE, "End Of File");
                }
                else {
                    return false;
                }
            }
            else {
                size -= r;
                ptr += r;
            }
        }
        return true;
    }




};


typedef InputStreamHandlerImplT<InputStreamHandler>		InputStreamHandlerImpl;


template <typename Interface>
class OutputStreamHandlerImplT: public Interface {
	typedef OutputStreamHandlerImplT<Interface>	Me;
	typedef Interface				Base;

public:
    virtual Int buffer_size() = 0;

    virtual void flush() = 0;
    virtual void close() = 0;
    virtual void write(const void* mem, int offset, int lenght) = 0;


};



typedef OutputStreamHandlerImplT<OutputStreamHandler>	OutputStreamHandlerImpl;



}
}


#endif	//_MEMORIA_VAPI_COLLECTIONS_ITERATOR_HPP
