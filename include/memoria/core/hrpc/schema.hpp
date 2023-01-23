
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
    static constexpr NamedCode CALL_ID                  = NamedCode(5, "callId");
protected:

public:
    Request() {}
    Request(hermes::TinyObjectMap object):
        hermes::TinyObjectBase(std::move(object))
    {}

    EndpointID endpoint() {
        return object_.get(PARAMETERS).cast_to<UBigInt>();
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
        auto vv = object_.get(INPUT_STREAMS);
        if (vv.is_not_empty()) {
            return vv.to_i32();
        }
        return 0;
    }

    uint64_t call_id() {
        return object_.get(CALL_ID).cast_to<UBigInt>();
    }

    void set_call_id(uint64_t call_id) {
        return object_.put(CALL_ID, call_id);
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

    Response compactify() {
        auto ctr = object_.ctr().compactify(true);
        return ctr.root().as_tiny_object_map();
    }
};

class StreamBatch {
protected:
    hermes::ObjectArray array_;
public:
    StreamBatch() {}

    StreamBatch(hermes::HermesCtr ctr):
        array_(ctr.make_object_array())
    {
        ctr.set_root(array_);
    }

    StreamBatch(hermes::ObjectArray&& array):
        array_(std::move(array))
    {}

    hermes::ObjectArray& array() {return array_;}
    const hermes::ObjectArray& array() const {return array_;}
};


class ConnectionMetadata: public hermes::TinyObjectBase {
public:
    static constexpr NamedCode VERSION = NamedCode(0, "version");

    ConnectionMetadata() {}

    ConnectionMetadata(hermes::TinyObjectMap&& object):
        hermes::TinyObjectBase(std::move(object))
    {}

    ConnectionMetadata(hermes::HermesCtr ctr):
        hermes::TinyObjectBase(ctr.make_tiny_map())
    {
        ctr.set_root(object_);
    }

    ProtocolVersion version() {
        return object_.get(VERSION).cast_to<UBigInt>().value_t();
    }

    void set_version(const ProtocolVersion& version) {
        object_.put(VERSION, version.value());
    }

    ConnectionMetadata compactify() {
        return object_.ctr().compactify(true);
    }
};


}
