
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_MD5_HPP_
#define MEMORIA_CORE_TOOLS_MD5_HPP_

#include <memoria/core/types/types.hpp>

namespace memoria {

class MD5Hash {
    static const Int BUFFER_SIZE = 16;

    typedef UInt (FunPtr)(UInt, UInt, UInt);

    class Quad {
        UInt A_;
        UInt B_;
        UInt C_;
        UInt D_;

        Quad(): A_(0), B_(0), C_(0), D_(0) {}
        Quad(UInt A, UInt B, UInt C, UInt D): A_(A), B_(B), C_(C), D_(D) {}
    public:
        Quad(const Quad& other): A_(other.A_), B_(other.B_), C_(other.C_), D_(other.D_) {}
    private:

        friend class MD5Hash;

        void add(const Quad& other) {
            A_ += other.A_;
            B_ += other.B_;
            C_ += other.C_;
            D_ += other.D_;
        }

    public:
        UBigInt hash64() const {
            return (((UBigInt)A_ << 32) | B_) ^ (((UBigInt)C_ << 32) | D_);
        }

        UInt hash32() const {
            return A_ ^ B_ ^ C_ ^ D_;
        }
    };

public:
    MD5Hash(): Q0_(0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476), ptr_(0), dirty_(true), accumulator_(0)
    {
        for (UInt& value: K_) value = 0;
    }

    void add(UInt value);
    void add_ubi(UBigInt value)
    {
    	this->add(static_cast<UInt>(value & 0xFFFFFFFF));
    	this->add(static_cast<UInt>((value >> 32) & 0xFFFFFFFF));
    }

    void compute();

    Quad result() {
        return Q0_;
    }

    void incAccumulator(BigInt amount) {
        accumulator_ += amount;
    }

    UInt getAccumulator() {
        return accumulator_;
    }

    void addAccumulator() {
        add(accumulator_);
    }

private:

    void round1();

    void round2();

    void round3();

    void round4();

    void Block(FunPtr Fun, Int k, Int s, Int i);

    void dumpState();

    UInt getBufferValue(Int k) {
        return K_[k];
    }

    UInt getMd5Constant(Int i) {
        return X[i - 1];
    }

    static UInt FunF (UInt X, UInt Y, UInt Z) {
        return (X & Y) | ((~X) & Z);
    };

    static UInt FunG (UInt X, UInt Y, UInt Z) {
        return (X & Z) | ((~Z) & Y);
    };

    static UInt FunH (UInt X, UInt Y, UInt Z) {
        return (X ^ Y ^ Z);
    };

    static UInt FunI (UInt X, UInt Y, UInt Z) {
        return Y ^ ((~Z) | X);
    };

    static UInt CShl(UInt Value, Int N) {
        return (Value << N) | (Value >> (sizeof(UInt) * 8 - N));
    };


    static const UInt X[64];
    UInt    K_[BUFFER_SIZE];
    Quad    Q0_;
    Int     ptr_;
    bool    dirty_;
    UInt    accumulator_;
};






}

#endif
