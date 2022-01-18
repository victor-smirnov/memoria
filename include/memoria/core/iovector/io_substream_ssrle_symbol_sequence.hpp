
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/packed/sseq/packed_ssrle_searchable_seq.hpp>

#include <memoria/core/memory/malloc.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <functional>

namespace memoria {
namespace io {


struct IOSSRLESymbolSequence: IOSubstream {

    virtual bool is_indexed() const                 = 0;
    virtual size_t alphabet_size() const            = 0;
    virtual bool is_const() const                   = 0;


    virtual size_t symbol(size_t idx) const         = 0;
    virtual size_t size() const                     = 0;

    virtual void reindex()                          = 0;
    virtual void dump(std::ostream& out) const      = 0;

    virtual void rank_to(uint64_t idx, Span<uint64_t> values) const  = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IOSSRLESymbolSequence>::name();
    }

    virtual const std::type_info& sequence_type() const = 0;
    virtual const std::type_info& substream_type() const {
        return typeid(IOSSRLESymbolSequence);
    }

    virtual void reset()                            = 0;
    virtual void configure(const void* ptr)         = 0;

    virtual void append_from(const IOSSRLESymbolSequence& source, size_t start, size_t length) = 0;
};




template <int32_t Bps>
class PackedSSRLESymbolSequence: public IOSSRLESymbolSequence {

    static_assert (Bps >= 1 && Bps <= 8, "");

    static constexpr size_t AlphabetSize = 1 << Bps;

    using SeqT = PkdSSRLESeqT<Bps>;

    SeqT* sequence_;

public:
    PackedSSRLESymbolSequence()
    {
        size_t size = SeqT::empty_size();
        sequence_ = ptr_cast<SeqT>(allocate_system<uint8_t>(size).release());
        sequence_->allocatable().setTopLevelAllocator();
        (void)sequence_->init();
    }

    PackedSSRLESymbolSequence(PackedSSRLESymbolSequence&&) = delete;
    PackedSSRLESymbolSequence(const PackedSSRLESymbolSequence&) = delete;

    virtual ~PackedSSRLESymbolSequence() noexcept
    {
        free_system(sequence_);
    }

    virtual bool is_indexed() const {
        return true;
    }

    virtual size_t alphabet_size() const {
        return AlphabetSize;
    }

    virtual bool is_const() const {
        return false;
    }

    virtual size_t symbol(size_t idx) const {
        return 0;//sequence_->symbol(idx);
    }

    virtual uint64_t size() const {
        return sequence_->size();
    }


    virtual void rank_to(uint64_t idx, Span<uint64_t> values) const
    {
        for (int32_t sym = 0; sym < AlphabetSize; sym++) {
            //values[sym] = sequence_->rank(idx, sym);
        }
    }

    virtual void append(Span<const SSRLERun<Bps>> runs)
    {

        while (true) {
            size_t required = sequence_->append(runs).get_or_throw();
            if (required > 0) {
                enlarge(required);
            }
            else {
                break;
            }
        }
    }


    virtual void check()
    {
        sequence_->check().get_or_throw();
    }

    virtual void reindex()
    {
        while (is_packed_error(sequence_->reindex())) {
            enlarge();
        }
    }

    virtual void reset() {
        sequence_->reset();
    }

    virtual void dump(std::ostream& out) const
    {
        DumpStruct(sequence_, out);
    }

    virtual const std::type_info& sequence_type() const {
        return typeid(SeqT);
    }


    void append_from(const IOSSRLESymbolSequence& source, size_t start, size_t length)
    {
//        SeqT* source_seq = ptr_cast<SeqT>(source.buffer());

//        while (is_packed_error(sequence_->insert_from(sequence_->size(), source_seq, start, length))) {
//            enlarge();
//        }
    }




    void configure(const void* ptr) {
        MMA_THROW(UnsupportedOperationException());
    }

private:

    template <typename T>
    bool is_packed_error(Result<T>&& res)
    {
        if (res.is_error()) {
            if (res.is_packed_error()) {
                return true;
            }
            else {
                res.get_or_throw();
            }
        }

        return false;
    }

    void enlarge()
    {
        int32_t bs = sequence_->block_size();
        SeqT* new_seq = ptr_cast<SeqT>(allocate_system<uint8_t>(bs * 2).release());
        memcpy(new_seq, sequence_, bs);
        new_seq->set_block_size(bs * 2);

        free_system(sequence_);

        sequence_ = new_seq;
    }

    void enlarge(size_t syms_block_size)
    {
        size_t current_bs = sequence_->block_size();
        size_t bs = SeqT::block_size(syms_block_size);
        SeqT* new_seq = ptr_cast<SeqT>(allocate_system<uint8_t>(bs).release());
        memcpy(new_seq, sequence_, current_bs);
        new_seq->set_block_size(bs);
        new_seq->resizeBlock(SeqT::SYMBOLS, syms_block_size).get_or_throw();

        free_system(sequence_);

        sequence_ = new_seq;
    }


    virtual U8String describe() const {
        return TypeNameFactory<PackedSSRLESymbolSequence>::name();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(PackedSSRLESymbolSequence);
    }
};







static inline std::unique_ptr<IOSSRLESymbolSequence> make_packed_ssrle_symbol_sequence(int32_t bps)
{
    if (bps == 1) {
        return std::make_unique<PackedSSRLESymbolSequence<1>>();
    }
//    else if (alphabet_size == 2) {
//        return std::make_unique<PackedSSRLESymbolSequence<2>>();
//    }
//    else if (alphabet_size == 3) {
//        return std::make_unique<PackedSSRLESymbolSequence<3>>();
//    }
//    else if (alphabet_size == 4) {
//        return std::make_unique<PackedSSRLESymbolSequence<4>>();
//    }
//    else if (alphabet_size == 5) {
//        return std::make_unique<PackedSSRLESymbolSequence<5>>();
//    }
//    else if (alphabet_size == 6) {
//        return std::make_unique<PackedSSRLESymbolSequence<6>>();
//    }
//    else if (alphabet_size == 7) {
//        return std::make_unique<PackedSSRLESymbolSequence<7>>();
//    }
//    else if (alphabet_size == 8) {
//        return std::make_unique<PackedSSRLESymbolSequence<8>>();
//    }
    else {
        MMA_THROW(RuntimeException()) << format_ex("Unsupported bits per symbol value: {}", bps);
    }
}

}}
