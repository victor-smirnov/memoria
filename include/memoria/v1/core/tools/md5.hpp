
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

#include <memoria/v1/core/types.hpp>

namespace memoria {
namespace v1 {

class MD5Hash {
    static const int32_t BUFFER_SIZE = 16;

    typedef uint32_t (FunPtr)(uint32_t, uint32_t, uint32_t);

    class Quad {
        uint32_t A_;
        uint32_t B_;
        uint32_t C_;
        uint32_t D_;

        Quad(): A_(0), B_(0), C_(0), D_(0) {}
        Quad(uint32_t A, uint32_t B, uint32_t C, uint32_t D): A_(A), B_(B), C_(C), D_(D) {}
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
        uint64_t hash64() const {
            return (((uint64_t)A_ << 32) | B_) ^ (((uint64_t)C_ << 32) | D_);
        }

        uint32_t hash32() const {
            return A_ ^ B_ ^ C_ ^ D_;
        }
    };

public:
    MD5Hash(): Q0_(0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476), ptr_(0), dirty_(true), accumulator_(0)
    {
        for (uint32_t& value: K_) value = 0;
    }

    void add(uint32_t value);
    void add_ubi(uint64_t value)
    {
        this->add(static_cast<uint32_t>(value & 0xFFFFFFFF));
        this->add(static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF));
    }

    void compute();

    Quad result() {
        return Q0_;
    }

    void incAccumulator(int64_t amount) {
        accumulator_ += amount;
    }

    uint32_t getAccumulator() {
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

    void Block(FunPtr Fun, int32_t k, int32_t s, int32_t i);

    void dumpState();

    uint32_t getBufferValue(int32_t k) {
        return K_[k];
    }

    uint32_t getMd5Constant(int32_t i) {
        return X[i - 1];
    }

    static uint32_t FunF (uint32_t X, uint32_t Y, uint32_t Z) {
        return (X & Y) | ((~X) & Z);
    };

    static uint32_t FunG (uint32_t X, uint32_t Y, uint32_t Z) {
        return (X & Z) | ((~Z) & Y);
    };

    static uint32_t FunH (uint32_t X, uint32_t Y, uint32_t Z) {
        return (X ^ Y ^ Z);
    };

    static uint32_t FunI (uint32_t X, uint32_t Y, uint32_t Z) {
        return Y ^ ((~Z) | X);
    };

    static uint32_t CShl(uint32_t Value, int32_t N) {
        return (Value << N) | (Value >> (sizeof(uint32_t) * 8 - N));
    };


    static const uint32_t X[64];
    uint32_t    K_[BUFFER_SIZE];
    Quad    Q0_;
    int32_t     ptr_;
    bool    dirty_;
    uint32_t    accumulator_;
};






}}
