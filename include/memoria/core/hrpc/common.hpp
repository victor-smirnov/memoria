
// Copyright 2023 Victor Smirnov
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

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/uid_256.hpp>

#include <memoria/core/memory/memory.hpp>

#include <array>

namespace memoria::hrpc {

using EndpointID    = UID256;
using CallID        = uint64_t;
using SessionID     = UID256;
using ShardID       = uint64_t;
using ChannelCode   = uint16_t;


class Context;
class EndpointRepoitory;
class Call;
class Session;
class Client;
class Server;


class ProtocolVersion {
    uint32_t value_;
public:
    constexpr ProtocolVersion(): value_() {}
    constexpr ProtocolVersion(uint32_t value): value_(value) {}

    constexpr uint32_t value() const {
        return value_;
    }

    constexpr bool operator<(const ProtocolVersion& other) const noexcept {
        return value_ < other.value_;
    }

    constexpr bool operator==(const ProtocolVersion& other) const noexcept {
        return value_ == other.value_;
    }

    constexpr bool operator!=(const ProtocolVersion& other) const noexcept {
        return value_ != other.value_;
    }
};

constexpr ProtocolVersion PROTOCOL_VERSION = ProtocolVersion(1);

enum class SessionSide: uint8_t {
    CLIENT, SERVER
};


enum class MessageType: uint8_t {
    SESSION_START,
    SESSION_CLOSE,
    CALL,
    RETURN,
    CALL_CHANNEL_MESSAGE,
    CONTEXT_CHANNEL_MESSAGE,
    CALL_CLOSE_INPUT_CHANNEL,
    CONTEXT_CLOSE_INPUT_CHANNEL,
    CALL_CLOSE_OUTPUT_CHANNEL,
    CONTEXT_CLOSE_OUTPUT_CHANNEL,
    CALL_CHANNEL_BUFFER_RESET,
    CONTEXT_CHANNEL_BUFFER_RESET,
    CANCEL_CALL
};


namespace detail {

static constexpr uint64_t compute_optf_sizes()
{
    constexpr size_t NUMF = 4;
    constexpr size_t sizes[NUMF]{sizeof(SessionID), sizeof(EndpointID), sizeof(ShardID), 0};

    uint64_t storage = 0;
    for (size_t mask = 0; mask < 1 << (NUMF - 1); mask++)
    {
        uint64_t size = 0;
        for(size_t idx = 0; idx < NUMF; idx++)
        {
            auto bit = (mask >> idx) & 0x1;
            size += sizes[idx] * bit;
        }

        storage |= (size / 8) << (mask * 4);
    }

    return storage;
}

constexpr uint64_t OPTF_SIZES  = compute_optf_sizes();



template <typename T> class OptField {
    const size_t field_num_;

public:
    constexpr OptField(size_t field_num):
        field_num_(field_num)
    {}

    template <typename MessageHeader>
    const T& get(const MessageHeader* host, uint64_t bits) const
    {
        if (bits & (1ull << field_num_))
        {
            const uint8_t* addr = reinterpret_cast<const uint8_t*>(host) + sizeof(MessageHeader);
            return *ptr_cast<const T>(addr + offset(field_num_, bits));
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Reading missing MessageHeader optional filed: {}", field_num_
            ).do_throw();
        }
    }

    template <typename MessageHeader>
    const void set(MessageHeader* host, uint64_t bits, const T& value) const
    {
        if (bits & (1ull << field_num_))
        {
            uint8_t* addr = reinterpret_cast<uint8_t*>(host) + sizeof(MessageHeader);
            *ptr_cast<T>(addr + offset(field_num_, bits)) = value;
        }
    }

private:
    static constexpr size_t value(uint64_t vals, uint64_t bits) {
        return (vals >> (bits * 4)) & 0xF;
    }

    static constexpr size_t offset(size_t idx, uint64_t bits)
    {
        uint64_t mask = (1ull << idx) - 1;
        return value(OPTF_SIZES, bits & mask) * 8;
    }
};

}

enum class HeaderOptF: uint32_t {
    SESSION_ID = 1, ENDPOINT_ID = 2, SHARD_ID = 4
};

inline uint32_t operator|(HeaderOptF l, HeaderOptF r) {
    return (uint32_t)l | (uint32_t)r;
}

inline uint32_t operator|(uint32_t l, HeaderOptF r) {
    return l | (uint8_t)r;
}

inline uint32_t operator|(HeaderOptF l, uint32_t r) {
    return (uint32_t)l | r;
}


class alignas(8) MessageHeader {

    static constexpr BitField MESSAGE_TYPE = BitField(0, 6);
    static constexpr BitField DURABILITY   = BitField(6, 1);
    static constexpr BitField ORDERING     = BitField(7, 1);
    static constexpr BitField CHANNEL_CODE = BitField(8, 16);
    static constexpr BitField OPTIONALS    = BitField(24, 4);
    static constexpr BitField EXTRA_DATA   = BitField(28, 4);


    static constexpr uint64_t OPTF_SIZES  = detail::OPTF_SIZES;

    using BitFieldsT = uint32_t;;

    static constexpr detail::OptField<SessionID>  SESSION_ID  = detail::OptField<SessionID>(0);
    static constexpr detail::OptField<EndpointID> ENDPOINT_ID = detail::OptField<EndpointID>(1);
    static constexpr detail::OptField<ShardID>    SHARD_ID    = detail::OptField<ShardID>(2);

    uint32_t message_size_;
    BitFieldsT bits_;
    CallID call_id_;

public:
    MessageHeader():
        message_size_(),
        bits_(),
        call_id_()
    {}

    MessageHeader(uint32_t opt_fields):
        message_size_(),
        bits_(),
        call_id_()
    {
        OPTIONALS.set(bits_, opt_fields);
    }

    uint32_t message_size() const {return message_size_;}
    MessageType message_type() const {return (MessageType) MESSAGE_TYPE.get(bits_);}
    ChannelCode channel_code() const {return CHANNEL_CODE.get(bits_);}
    CallID call_id() const {return call_id_;}

    SessionID session_id() const {
        return SESSION_ID.get(this, OPTIONALS.get(bits_));
    }

    EndpointID endpoint_id() const {
        return ENDPOINT_ID.get(this, OPTIONALS.get(bits_));
    }

    ShardID shard_id() const {
        return SHARD_ID.get(this, OPTIONALS.get(bits_));
    }


    void set_message_size(uint32_t size) {
        message_size_ = size;
    }

    void set_message_type(MessageType type){
        MESSAGE_TYPE.set(bits_, type);
    }

    void set_call_id(CallID id) {
        call_id_ = id;
    }

    void set_channel_code(ChannelCode code) {
        CHANNEL_CODE.set(bits_, code);
    }

    void set_opt_session_id(const SessionID& id) {
        SESSION_ID.set(this, OPTIONALS.get(bits_), id);
    }

    void set_opt_endpoint_id(const EndpointID& id) {
        ENDPOINT_ID.set(this, OPTIONALS.get(bits_), id);
    }

    void set_opt_shard_id(const ShardID& id) {
        SHARD_ID.set(this, OPTIONALS.get(bits_), id);
    }

    size_t header_size() const {
        return value(OPTF_SIZES, OPTIONALS.get(bits_)) + basic_size();
    }

    static constexpr size_t header_size_for(BitFieldsT optionals) {
        return value(OPTF_SIZES, optionals) + basic_size();
    }

    size_t docuemnt_size() const {
        return message_size_ - header_size();
    }

    static constexpr size_t basic_size() {
        return sizeof(MessageHeader);
    }

private:
    static constexpr size_t value(uint64_t vals, uint64_t bits) {
        return ((vals >> (bits * 4)) & 0xF) * 8;
    }
};

using RawMessagePtr = UniquePtr<uint8_t>;

}
