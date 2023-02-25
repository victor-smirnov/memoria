
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
    static constexpr NamedCode PARAMETERS               = NamedCode(1, "parameters");
    static constexpr NamedCode INPUT_CHANNELS           = NamedCode(2, "input_channels");
    static constexpr NamedCode OUTPUT_CHANNELS          = NamedCode(3, "output_channels");
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

    hermes::TinyObjectMap parameters() {
        return object_.get(PARAMETERS).as_tiny_object_map();
    }

    ChannelCode input_channels() {
        auto vv = object_.get(INPUT_CHANNELS);
        if (vv.is_not_empty()) {
            return vv.to_i32();
        }
        return 0;
    }

    ChannelCode output_channels() {
        auto vv = object_.get(OUTPUT_CHANNELS);
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

    template <typename T>
    void set_parameter(NamedCode name, T param)
    {
        hermes::Object params_obj = object_.get(PARAMETERS);
        if (params_obj.is_null()) {
            params_obj = object_.ctr().make_tiny_map();
            object_.put(PARAMETERS, params_obj);
        }

        return params_obj.as_tiny_object_map().put(name, param);
    }

    void set_input_channels(ChannelCode num) {
        object_.put(INPUT_CHANNELS, num);
    }

    void set_output_channels(ChannelCode num) {
        object_.put(OUTPUT_CHANNELS, num);
    }

    static Request make() {
        Request rq(hermes::HermesCtr::make_new());
        return rq;
    }

    Request compactify() const {
        auto ctr = object_.ctr().compactify(true);
        return ctr.root().as_tiny_object_map();
    }
};

enum class ErrorType: uint32_t {
    MEMORIA, HRPC, BOOST, CXX_STD, CXX_SYSTEM, UNKNOWN
};


class Error: public hermes::TinyObjectBase {
public:
    static constexpr NamedCode TYPE         = NamedCode(1, "type");
    static constexpr NamedCode DESCRIPTION  = NamedCode(2, "description");
public:
    Error() {}
    Error(hermes::TinyObjectMap object):
        hermes::TinyObjectBase(std::move(object))
    {}

    ErrorType type() const {
        return (ErrorType)object_.get(TYPE).cast_to<UInteger>().value_t();
    }

    U8StringOView description() const {
        return object_.get(DESCRIPTION).cast_to<Varchar>();
    }

    void set_type(ErrorType type) {
        object_.put_t<UInteger>(TYPE, (uint32_t)type);
    }

    void set_description(U8StringView descr) {
        object_.put_t<Varchar>(DESCRIPTION, descr);
    }
};

class CxxSystemError: public Error {
public:
    static constexpr NamedCode CODE                 = NamedCode(3, "code");
    static constexpr NamedCode MESSAGE              = NamedCode(4, "message");
    static constexpr NamedCode CATEGORY             = NamedCode(5, "category");
    static constexpr NamedCode CONDITION_CODE       = NamedCode(6, "condition_code");
    static constexpr NamedCode CONDITION_MESSAGE    = NamedCode(7, "condition_message");
    static constexpr NamedCode CONDITION_CATEGORY   = NamedCode(8, "condition_category");

public:
    CxxSystemError() {}
    CxxSystemError(hermes::TinyObjectMap object):
        Error(std::move(object))
    {}

    CxxSystemError(hermes::HermesCtr ctr, const std::system_error& err):
        Error(std::move(ctr.make_tiny_map()))
    {
        set_type(ErrorType::CXX_SYSTEM);
        set_description(err.what());

        set_code(err.code().value());
        set_message(err.code().message());
        set_categorty(err.code().category().name());

        set_condition_code(err.code().default_error_condition().value());
        set_condition_message(err.code().default_error_condition().message());
        set_condition_categorty(err.code().default_error_condition().category().name());
    }

    int32_t code() const {
        return object_.get(CODE).cast_to<Integer>();
    }

    U8StringOView message() const {
        return object_.get(MESSAGE).cast_to<Varchar>();
    }

    U8StringOView category() const {
        return object_.get(CATEGORY).cast_to<Varchar>();
    }

    int32_t condition_code() const {
        return object_.get(CONDITION_CODE).cast_to<Integer>();
    }

    U8StringOView condition_message() const {
        return object_.get(CONDITION_MESSAGE).cast_to<Varchar>();
    }

    U8StringOView condition_category() const {
        return object_.get(CONDITION_CATEGORY).cast_to<Varchar>();
    }

    void set_code(int32_t code) {
        object_.put_t<Integer>(CODE, (int32_t)code);
    }

    void set_message(U8StringView txt) {
        object_.put_t<Varchar>(MESSAGE, txt);
    }

    void set_categorty(U8StringView cat) {
        object_.put_t<Varchar>(CATEGORY, cat);
    }

    void set_condition_code(int32_t code) {
        object_.put_t<Integer>(CONDITION_CODE, (int32_t)code);
    }

    void set_condition_message(U8StringView txt) {
        object_.put_t<Varchar>(CONDITION_MESSAGE, txt);
    }

    void set_condition_categorty(U8StringView cat) {
        object_.put_t<Varchar>(CONDITION_CATEGORY, cat);
    }
};

enum class HrpcErrors: int32_t {
    INVALID_ENDPOINT,
    OTHER
};

class HrpcError: public Error {
public:
    static constexpr NamedCode CODE = NamedCode(3, "code");
    static constexpr NamedCode ENDPOINT_ID = NamedCode(4, "endpoint_id");

    HrpcError() {}
    HrpcError(hermes::TinyObjectMap object):
        Error(std::move(object))
    {}

    HrpcErrors code() const {
        return (HrpcErrors)object_.get(CODE).to_i32();
    }

    Optional<EndpointID> endpoint_id() const
    {
        auto obj = object_.get(ENDPOINT_ID);
        if (obj.is_not_empty()) {
            return obj.cast_to<typename ViewToDTMapping<EndpointID>::Type>().value_t();
        }
        else {
            return {};
        }
    }

    void set_code(HrpcErrors code) {
        object_.put_t<Integer>(CODE, (int32_t)code);
    }

    void set_endpoint_id(const EndpointID& endpoint_id) {
        object_.put(ENDPOINT_ID, endpoint_id);
    }
};



enum class StatusCode: uint32_t {
    OK = 0, ERROR
};

struct Response: public hermes::TinyObjectBase {
    static constexpr NamedCode RESULT       = NamedCode(1, "result");
    static constexpr NamedCode STATUS_CODE  = NamedCode(2, "status_code");
    static constexpr NamedCode ERROR        = NamedCode(3, "error");
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

    template <typename T>
    void set_result(T&& result) {
        object_.put(RESULT, std::forward<T>(result));
    }

    template <typename DT, typename ViewT>
    void set_result_t(ViewT&& result) {
        object_.put_t<DT>(RESULT, std::forward<ViewT>(result));
    }

    void set_error(const Error& error) {
        object_.put(ERROR, error.object());
    }

    Error set_error()
    {
        Error error(object_.ctr().make_tiny_map());
        object_.put(ERROR, error.object());
        return error;
    }

    Error set_error(ErrorType type)
    {
        Error error(object_.ctr().make_tiny_map());
        error.set_type(type);
        object_.put(ERROR, error.object());
        return error;
    }

    Error set_error(ErrorType type, U8StringView descr)
    {
        Error error(object_.ctr().make_tiny_map());
        error.set_type(type);
        error.set_description(descr);
        object_.put(ERROR, error.object());
        return error;
    }

    CxxSystemError set_system_error()
    {
        CxxSystemError error(object_.ctr().make_tiny_map());
        error.set_type(ErrorType::CXX_SYSTEM);
        object_.put(ERROR, error.object());
        return error;
    }

    CxxSystemError set_error(const std::system_error& err)
    {
        CxxSystemError error(object_.ctr(), err);
        object_.put(ERROR, error.object());
        return error;
    }

    HrpcError set_hrpc_error(HrpcErrors code)
    {
        HrpcError error(object_.ctr().make_tiny_map());
        error.set_type(ErrorType::HRPC);
        error.set_code(code);
        object_.put(ERROR, error.object());
        return error;
    }

    HrpcError set_hrpc_error(HrpcErrors code, U8StringView descr)
    {
        auto error = set_hrpc_error(code);
        error.set_description(descr);
        return error;
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

class Message: public hermes::TinyObjectBase {
protected:

public:
    static constexpr NamedCode DATA       = NamedCode(1, "data");
    static constexpr NamedCode METADATA   = NamedCode(2, "metadata");

    Message() {}

    Message(hermes::TinyObjectMap&& object):
        hermes::TinyObjectBase(std::move(object))
    {}

    Message(hermes::HermesCtr ctr):
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

    Message compactify() const {
        return object_.ctr().compactify(true).root().as_tiny_object_map();
    }

    static Message empty() {
        Message batch(hermes::HermesCtr::make_pooled());
        return batch;
    }
};


class ConnectionMetadata: public hermes::TinyObjectBase {

public:
    static constexpr NamedCode CHANNEL_BUFFER_SIZE = NamedCode(1, "channel_buffer_size");

    ConnectionMetadata() {}

    ConnectionMetadata(hermes::TinyObjectMap&& object):
        hermes::TinyObjectBase(std::move(object))
    {}

    ConnectionMetadata(hermes::HermesCtr&& ctr):
        hermes::TinyObjectBase(std::move(ctr))
    {}

    uint64_t channel_buffer_size() const {
        return object_.get(CHANNEL_BUFFER_SIZE).cast_to<UBigInt>();
    }

    void set_channel_buffer_size(uint64_t size) {
        object_.put(CHANNEL_BUFFER_SIZE, size);
    }

    ConnectionMetadata compactify(size_t header_size = 0) const {
        return object_.ctr().compactify(true, header_size).root().as_tiny_object_map();
    }
};

class ProtocolConfig: public hermes::TinyObjectBase {
public:
    static constexpr NamedCode CHANNEL_BUFFER_SIZE = NamedCode(1, "channel_buffer_size");
    static constexpr uint64_t  CHANNEL_BUFFER_SIZE_DEFAULT = 1024*1024; // 1MB

    ProtocolConfig() {}
    ProtocolConfig(hermes::TinyObjectMap map):
        hermes::TinyObjectBase(std::move(map))
    {}

    ProtocolConfig(hermes::HermesCtr&& ctr):
        hermes::TinyObjectBase(std::move(ctr))
    {}

    uint64_t channel_buffer_size() const
    {
        auto val = object_.get(CHANNEL_BUFFER_SIZE);
        if (val.is_not_null()) {
            return val.cast_to<UBigInt>();
        }

        return CHANNEL_BUFFER_SIZE_DEFAULT;
    }

    void set_channel_buffer_size(uint64_t size) {
        object_.put(CHANNEL_BUFFER_SIZE, size);
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
        TCPClientSocketConfig cfg(hermes::HermesCtr::make_new());
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
