
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_PMAP_PACKED_MAP_INTRNL_H
#define _MEMORIA_CORE_TOOLS_PMAP_PACKED_MAP_INTRNL_H

#include <memoria/vapi/metadata.hpp>
#include <memoria/core/types/traits.hpp>

#include <iostream>
#include <typeinfo>
#include <stddef.h>

namespace memoria        {
namespace vlen			 {


template <typename Key>
class Accumulator {
    Key key_;
public:

    typedef Key key_t;

    Accumulator(): key_(0) {}
    Accumulator(const Key& key): key_(key) {}

    void operator()(const Key& k) {
        key_ += k;
    };

    const Key& get() const {
        return key_;
    }

    void Reset() {
        key_.Clear();
    }

    void Reset(const Key& k) {
        key_ = k;
    }

    void Sub(const Key& k) {
        key_ -= k;
    }

    bool isShouldReset() const {
    	return true;
    }
};

template <typename Key>
class Tracker {
    Key key_;
public:
    Tracker(): key_(0) {}
    Tracker(const Key& key): key_(key) {}

    void operator()(const Key& k) {
        key_ = k;
    }

    const Key& get() const {
        return key_;
    }

    void Reset() {
        key_.Clear();
    }
};

template <typename Key>
class CompareLT {
public:
    bool operator()(const Key& k0, const Key& k1) const {
        return k0 < k1;
    }

    void Reset() {}
    void Sub(const Key& k) {}
    void Reset(const Key& k) {}

    bool isShouldReset() const {
    	return false;
    }

    Key get() const {
        return Key(0, NULL);
    }

    bool TestMax(const Key& k, const Key& max) const {
        return k >= max;
    }
};

template <typename Key>
class CompareLTAcc: public Accumulator<Key> {
	typedef Accumulator<Key> Base;

public:
    CompareLTAcc(): Base() {}

    bool operator()(const Key& k0, const Key& k1)
    {
        Base::operator()(k1);
        return k0 < Base::get();
    }

    bool TestMax(const Key& k, const Key& max) const {
        return k >= max;
    }
};

template <typename Key>
class CompareLE {
public:
    bool operator()(const Key& k0, const Key& k1) const {
        return k0 <= k1;
    }

    void Reset() {}
    void Sub(const Key& k) {}
    void Reset(const Key& k) {}

    bool isShouldReset() const {
    	return false;
    }

    Key get() const {
        return Key(0,NULL);
    }

    bool TestMax(const Key& k, const Key& max) const {
        return k > max;
    }
};

template <typename Key>
class CompareLEAcc: public Accumulator<Key> {
	typedef Accumulator<Key> Base;
public:
    CompareLEAcc(): Base() {}
    bool operator()(const Key& k0, const Key& k1) {
        Base::operator()(k1);
        return k0 <= Base::get();
    }

    bool TestMax(const Key& k, const Key& max) const {
        return k > max;
    }
};

template <typename Key>
class CompareEQ {
public:
    bool operator()(const Key& k0, const Key& k1) const {
        return k0 == k1;
    }
    void Reset() {}
    void Sub(const Key& k) {}
    void Reset(const Key& k) {}

    bool isShouldReset() const {
    	return false;
    }

    Key get() const {
        return Key(0, NULL);
    }

    bool TestMax(const Key& k, const Key& max) const {
        return k > max;
    }
};

template <typename Key>
class CompareEQAcc: public Accumulator<Key> {
	typedef Accumulator<Key> Base;
public:
    CompareEQAcc(): Accumulator<Key>() {}
    bool operator()(const Key& k0, const Key& k1) {
        Base::operator()(k1);
        return k0 == Base::get();
    }

    bool TestMax(const Key& k, const Key& max) const {
        return k > max;
    }
};

struct VoidValue {
    static const int SIZE = 0;
    static const Int kHashCode = 123151;
    VoidValue() {}
};

}
} //memoria


#endif
