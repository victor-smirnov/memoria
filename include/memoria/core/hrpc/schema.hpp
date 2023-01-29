
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

#include <memoria/core/hrpc/common.hpp>
#include <memoria/core/hermes/hermes.hpp>

namespace memoria::hrpc {


struct Request: public hermes::TinyObjectBase {
    static constexpr NamedCode ENDPOINT                 = NamedCode(0, "endpoint");
    static constexpr NamedCode PARAMETERS               = NamedCode(1, "parameters");
    static constexpr NamedCode INPUT_STREAMS            = NamedCode(2, "inputStreams");
    static constexpr NamedCode OUTPUT_STREAMS           = NamedCode(3, "outputStreams");
    static constexpr NamedCode METADATA                 = NamedCode(4, "metadata");
protected:

public:
    Request() {}
    Request(hermes::TinyObjectMap object):
        hermes::TinyObjectBase(std::move(object))
    {}

    Request(hermes::HermesCtr&& ctr):
        hermes::TinyObjectBase(ctr.make_tiny_map())
    {
        ctr.set_root(object_);
    }

    EndpointID endpoint() {
        return object_.get(ENDPOINT).cast_to<UBigInt>();
    }

    void set_endpoint(EndpointID endpoint) {
        object_.put(ENDPOINT, endpoint);
    }

    hermes::TinyObjectMap parameters() {
        return object_.get(PARAMETERS).as_tiny_object_map();
    }

    StreamCode input_streams() {
        auto vv = object_.get(INPUT_STREAMS);
        if (vv.is_not_empty()) {
            return vv.to_i32();
        }
        return 0;
    }

    StreamCode output_streams() {
        auto vv = object_.get(OUTPUT_STREAMS);
        if (vv.is_not_empty()) {
            return vv.to_i32();
        }
        return 0;
    }

    hermes::Object metadata() {
        return object_.get(METADATA);
    }

    void set_metadata(const hermes::Object& metadata) {
        return object_.put(METADATA, metadata);
    }

    void set_parameters(const hermes::TinyObjectMap& params) {
        return object_.put(PARAMETERS, params);
    }

    void set_input_streams(StreamCode streams_num) {
        object_.put(INPUT_STREAMS, streams_num);
    }

    void set_output_streams(StreamCode streams_num) {
        object_.put(OUTPUT_STREAMS, streams_num);
    }

    static Request make() {
        Request rq(hermes::HermesCtr::make_pooled());
        return rq;
    }

    Request compactify() const {
        auto ctr = object_.ctr().compactify(true);
        return ctr.root().as_tiny_object_map();
    }
};

enum class ErrorCode: uint32_t {
    GENERIC = 0
};


class Error: public hermes::TinyObjectBase {
public:
    static constexpr NamedCode CODE         = NamedCode(0, "code");
    static constexpr NamedCode DESCRIPTION  = NamedCode(1, "description");
protected:    
public:
    Error() {}
    Error(hermes::TinyObjectMap object):
        hermes::TinyObjectBase(std::move(object))
    {}

    ErrorCode code() const {
        return (ErrorCode)object_.get(CODE).cast_to<UInteger>().value_t();
    }

    U8StringOView description() const {
        return object_.get(DESCRIPTION).cast_to<Varchar>();
    }

    void set_code(ErrorCode code) {
        object_.put_t<UInteger>(CODE, (uint32_t)code);
    }

    void set_description(U8StringView descr) {
        object_.put_t<Varchar>(CODE, descr);
    }
};


enum class StatusCode: uint32_t {
    OK = 0, ERROR
};

struct Response: public hermes::TinyObjectBase {
    static constexpr NamedCode RESULT       = NamedCode(0, "result");
    static constexpr NamedCode STATUS_CODE  = NamedCode(1, "status_code");
    static constexpr NamedCode ERROR        = NamedCode(1, "error");
protected:
public:
    Response() {}

    Response(hermes::TinyObjectMap&& object):
        hermes::TinyObjectBase(std::move(object))
    {}

    Response(hermes::HermesCtr ctr):
        hermes::TinyObjectBase(ctr.make_tiny_map())
    {
        ctr.set_root(object_);
    }


    hermes::Object result() const {
        return object_.get(RESULT);
    }

    StatusCode status_code() const {
        return (StatusCode)object_.get(STATUS_CODE).cast_to<UInteger>().value_t();
    }

    Error error() const {
        return object_.get(ERROR).as_tiny_object_map();
    }

    void set_status_code(StatusCode code) {
        object_.put(STATUS_CODE, (uint32_t)code);
    }

    void set_result(hermes::Object result) {
        object_.put(RESULT, result);
    }

    void set_error(const Error& error) {
        object_.put(ERROR, error.object());
    }

    static Response make_empty()
    {
        auto ctr = hermes::HermesCtr::make_pooled();
        return Response(ctr);
    }

    static Response ok()
    {
        auto rs = make_empty();
        rs.set_status_code(StatusCode::OK);
        return rs;
    }

    template <typename T>
    static Response ok(T&& value)
    {
        auto rs = ok();
        rs.set_result(rs.object().ctr().make(std::forward<T>(value)));
        return rs;
    }

    static Response error0()
    {
        auto rs = make_empty();
        rs.set_status_code(StatusCode::ERROR);
        return rs;
    }

    Response compactify() const {
        auto ctr = object_.ctr().compactify(true);
        return ctr.root().as_tiny_object_map();
    }
};

class StreamMessage: public hermes::TinyObjectBase {
protected:

public:
    static constexpr NamedCode DATA       = NamedCode(1, "data");
    static constexpr NamedCode METADATA   = NamedCode(2, "metadata");

    StreamMessage() {}

    StreamMessage(hermes::TinyObjectMap&& object):
        hermes::TinyObjectBase(std::move(object))
    {}

    StreamMessage(hermes::HermesCtr ctr):
        hermes::TinyObjectBase(ctr)
    {}


    hermes::Object metadata() const {
        return object_.get(METADATA);
    }

    void set_metadata(const hermes::Object& meta) {
        object_.put(METADATA, meta);
    }

    hermes::Object data() const {
        return object_.get(DATA);
    }

    void set_data(const hermes::Object& data) {
        object_.put(DATA, data);
    }

    U8String to_pretty_string() const {
        return object_.as_object().to_pretty_string();
    }

    StreamMessage compactify() const {
        return object_.ctr().compactify(true).root().as_tiny_object_map();
    }

    static StreamMessage empty() {
        StreamMessage batch(hermes::HermesCtr::make_pooled());
        return batch;
    }
};


class ConnectionMetadata: public hermes::TinyObjectBase {

public:
    static constexpr NamedCode STREAM_BUFFER_SIZE = NamedCode(1, "stream_buffer_size");

    ConnectionMetadata() {}

    ConnectionMetadata(hermes::TinyObjectMap&& object):
        hermes::TinyObjectBase(std::move(object))
    {}

    ConnectionMetadata(hermes::HermesCtr&& ctr):
        hermes::TinyObjectBase(std::move(ctr))
    {}

    uint64_t stream_buffer_size() const {
        return object_.get(STREAM_BUFFER_SIZE).cast_to<UBigInt>();
    }

    void set_stream_buffer_size(uint64_t size) {
        object_.put(STREAM_BUFFER_SIZE, size);
    }

    ConnectionMetadata compactify() const {
        return object_.ctr().compactify(true).root().as_tiny_object_map();
    }
};

class ProtocolConfig: public hermes::TinyObjectBase {
public:
    static constexpr NamedCode STREAM_BUFFER_SIZE = NamedCode(1, "stream_buffer_size");
    static constexpr uint64_t  STREAM_BUFFER_SIZE_DEFAULT = 1024*1024; // 1MB

    ProtocolConfig() {}
    ProtocolConfig(hermes::TinyObjectMap map):
        hermes::TinyObjectBase(std::move(map))
    {}

    ProtocolConfig(hermes::HermesCtr&& ctr):
        hermes::TinyObjectBase(std::move(ctr))
    {}

    uint64_t stream_buffer_size() const
    {
        auto val = object_.get(STREAM_BUFFER_SIZE);
        if (val.is_not_null()) {
            return val.cast_to<UBigInt>();
        }

        return STREAM_BUFFER_SIZE_DEFAULT;
    }

    void set_stream_buffer_size(uint64_t size) {
        object_.put(STREAM_BUFFER_SIZE, size);
    }
};

class TCPProtocolConfig: public ProtocolConfig {
public:
    static constexpr NamedCode      HOST = NamedCode(2, "host");
    static constexpr U8StringView   HOST_DEFAULT = "localhost";

    static constexpr NamedCode      PORT = NamedCode(3, "port");
    static constexpr uint16_t       PORT_DEFAULT = 3145;

    TCPProtocolConfig() {}
    TCPProtocolConfig(hermes::TinyObjectMap&& map):
        ProtocolConfig(std::move(map))
    {}

    TCPProtocolConfig(hermes::HermesCtr&& ctr):
        ProtocolConfig(std::move(ctr))
    {}

    U8String host() const
    {
        auto val = object_.get(HOST);
        if (val.is_not_null()) {
            return val.cast_to<Varchar>();
        }

        return HOST_DEFAULT;
    }

    void set_host(U8StringView value) {
        object_.put(HOST, value);
    }

    uint16_t port() const
    {
        auto val = object_.get(PORT);
        if (val.is_not_null()) {
            return val.cast_to<USmallInt>();
        }

        return PORT_DEFAULT;
    }

    void set_port(uint16_t value) {
        object_.put(PORT, value);
    }
};


class TCPClientSocketConfig: public TCPProtocolConfig {
public:
    TCPClientSocketConfig() {}
    TCPClientSocketConfig(hermes::TinyObjectMap&& map):
        TCPProtocolConfig(std::move(map))
    {}

    TCPClientSocketConfig(hermes::HermesCtr&& ctr):
        TCPProtocolConfig(std::move(ctr))
    {}

    static TCPClientSocketConfig of_host(U8StringView host, uint16_t port = TCPProtocolConfig::PORT_DEFAULT) {
        TCPClientSocketConfig cfg(hermes::HermesCtr::make_pooled());
        cfg.set_host(host);
        return cfg;
    }
};


class TCPServerSocketConfig: public TCPProtocolConfig {
public:
    TCPServerSocketConfig() {}
    TCPServerSocketConfig(hermes::TinyObjectMap&& map):
        TCPProtocolConfig(std::move(map))
    {}

    TCPServerSocketConfig(hermes::HermesCtr&& ctr):
        TCPProtocolConfig(std::move(ctr))
    {}

    static TCPServerSocketConfig of_host(U8StringView host, uint16_t port = TCPProtocolConfig::PORT_DEFAULT) {
        TCPServerSocketConfig cfg(hermes::HermesCtr::make_pooled());
        cfg.set_host(host);
        return cfg;
    }
};

}
