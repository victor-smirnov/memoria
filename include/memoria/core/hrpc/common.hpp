
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

namespace memoria::hrpc {

using EndpointID = uint64_t;
using CallID     = uint64_t;
using StreamCode = uint16_t;

class Context;
class HRPCService;
class HRPCCall;
class Connection;
class ClientConnection;
class ServerConnection;

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

constexpr uint16_t HRPC_PORT = 3145;

class TCPClinetSocketConfig {
    U8String host_;
    uint16_t port_;
public:
    TCPClinetSocketConfig(U8String host = "127.0.01", uint16_t port = HRPC_PORT):
        host_(host), port_(port)
    {}

    const U8String& host() const {return host_;}
    uint16_t port() const {return port_;}
};

enum class ConnectionType: uint8_t {
    CLIENT, SERVER
};


enum class MessageType: uint8_t {
    CONNECTION,
    CALL,
    RETURN,
    CALL_STREAM_MESSAGE,
    CONTEXT_STREAM_MESSAGE,
    CALL_CLOSE_INPUT_STREAM,
    CONTEXT_CLOSE_INPUT_STREAM,
    CALL_CLOSE_OUTPUT_STREAM,
    CONTEXT_CLOSE_OUTPUT_STREAM,
    CANCEL_CALL
};

class MessageHeader {
    uint32_t message_size_;
    MessageType message_type_;
    uint8_t reserved_;
    StreamCode stream_code_;
    CallID call_id_;
public:
    MessageHeader():
        message_size_(), message_type_(),
        reserved_(), stream_code_(),
        call_id_()
    {}

    uint32_t message_size() const {return message_size_;}
    MessageType message_type() const {return message_type_;}
    CallID call_id() const {return call_id_;}
    StreamCode stream_code() const {return stream_code_;}

    void set_message_size(uint32_t size) {
        message_size_ = size;
    }

    void set_message_type(MessageType type) {
        message_type_ = type;
    }

    void set_call_id(CallID id) {
        call_id_ = id;
    }

    void set_stream_code(StreamCode code) {
        stream_code_ = code;
    }
};

}
