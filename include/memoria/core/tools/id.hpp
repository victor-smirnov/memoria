
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_REFLECTION_ID_HPP
#define	_MEMORIA_VAPI_REFLECTION_ID_HPP


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>

namespace memoria    {
namespace vapi       {

class MEMORIA_API IDValue {
    Byte data_[8];
public:
    IDValue() {
        clear();
    }

    template <typename T>
    IDValue(const T* id) {
        clear();
        id->CopyTo(ptr());
    }

    IDValue(StringRef id) {
    	clear();
    }

    IDValue(BigInt id)
    {
    	BigInt* data_ptr = T2T<BigInt*>(data_);
    	*data_ptr = id;
    }

    void clear() {
        for (unsigned c = 0; c < sizeof(data_); c++) {
            data_[c] = 0;
        }
    }

    template <typename T>
    void set(const T& id) {
        clear();
        id.CopyTo(ptr());
    }

    template <typename T>
    const T get() const {
        return T(*this);
    }

    const void* ptr() const {
        return data_;
    }

    void* ptr() {
        return data_;
    }

    const char* cptr() const {
        return data_;
    }

    virtual const String str() const {
        char text[sizeof(data_)*2 + 3];
        text[0] = '0';
        text[1] = 'x';
        text[sizeof(text) - 1] = 0;

        for (unsigned c = 0; c < sizeof(data_); c++) {
            text[c*2 + 3] = get_char(data_[sizeof(data_) - c - 1] & 0xf);
            text[c*2 + 2] = get_char((data_[sizeof(data_) - c - 1] >> 4) & 0xf);
        }

        return String(text);
    }

    virtual bool IsNull() const {
        for (unsigned c = 0; c < sizeof(data_); c++) {
            if (data_[c] != 0) {
                return false;
            }
        }
        return true;
    }

    bool equals(const IDValue& other) const {
        for (unsigned c = 0; c < sizeof(data_); c++) {
            if (data_[c] != other.data_[c]) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const IDValue& other) const {
    	return equals(other);
    }

    bool operator<(const IDValue& other) const
    {
    	for (Int c = 7; c >=0; c--)
    	{
    		Int v0 = data_[c];
    		Int v1 = other.data_[c];
    		if (v0 != v1)
    		{
    			return v0 < v1;
    		}
    	}
    	return false;
    }

private:

    static char get_char(Byte value) {
        switch (value) {
            case 0: return '0';
            case 1: return '1';
            case 2: return '2';
            case 3: return '3';
            case 4: return '4';
            case 5: return '5';
            case 6: return '6';
            case 7: return '7';
            case 8: return '8';
            case 9: return '9';
            case 10: return 'a';
            case 11: return 'b';
            case 12: return 'c';
            case 13: return 'd';
            case 14: return 'e';
            case 15: return 'f';
        }
        return 'X';
    }
};

}
}

namespace std {

MEMORIA_API ostream& operator<<(ostream& os, const memoria::vapi::IDValue& id);

}
#endif

