
// Copyright 2018 Victor Smirnov
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
#include <memoria/v1/yaml-cpp/yaml.h>
#include <memoria/v1/filesystem/path.hpp>

#include <memoria/v1/core/tools/optional.hpp>

#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>



#include <string>
#include <iostream>

namespace memoria {
namespace v1 {
namespace tests {

struct ConfigurationContext {
    virtual ~ConfigurationContext() noexcept {}
    virtual filesystem::path resource_path(const std::string& name) = 0;
};

template <typename T> struct IndirectStateFiledSerializer;

template <>
struct IndirectStateFiledSerializer<U8String> {
    static void externalize(const U8String& value, filesystem::path path, ConfigurationContext* context){

    }

    static void internalize(U8String& value, filesystem::path path, ConfigurationContext* context) {

    }
};


struct FieldHandler {
    virtual ~FieldHandler() noexcept {}
    virtual void externalize(YAML::Node& node, ConfigurationContext* context) = 0;
    virtual void internalize(const YAML::Node& node, ConfigurationContext* context) = 0;
};

template <typename T>
class TypedFieldHandler: public FieldHandler {
    std::string name_;
    T& field_;
public:
    TypedFieldHandler(std::string name, T& field):
        name_(std::move(name)), field_(field)
    {}

    virtual void externalize(YAML::Node& node, ConfigurationContext* context) {
        node[name_] = field_;
    }

    virtual void internalize(const YAML::Node& node, ConfigurationContext* context)
    {
        if (node[name_]) {
            field_ = node[name_].template as<T>();
        }
    }
};

template <typename T>
class TypedIndirectFieldHandler: public FieldHandler {
    std::string name_;
    T& field_;
public:
    TypedIndirectFieldHandler(std::string name, T& field):
        name_(std::move(name)), field_(field)
    {}


    virtual void externalize(YAML::Node& node, ConfigurationContext* context)
    {
        auto path = context->resource_path(name_);
        node[name_] = path.to_u8();
        IndirectStateFiledSerializer<T>::externalize(field_, path, context);
    }

    virtual void internalize(const YAML::Node& node, ConfigurationContext* context)
    {
        if (node[name_])
        {
            auto file_name = node[name_].template as<std::string>();
            filesystem::path file_path = file_name;

            if (file_path.is_relative())
            {
                auto path = context->resource_path(file_name);
                IndirectStateFiledSerializer<T>::internalize(field_, path, context);
            }
            else {
                IndirectStateFiledSerializer<T>::internalize(field_, file_path, context);
            }
        }
    }
};

enum class TestCoverage {SMOKE, TINY, SMALL, MEDIUM, LARGE, XLARGE};

Optional<TestCoverage> coverage_from_string(const U8String& str);

class TestState {
    std::vector<std::unique_ptr<FieldHandler>> handlers_;
public:
    filesystem::path working_directory_;

    TestState() = default;
    TestState(TestState&&) = default;
    TestState(const TestState&) = delete;

    virtual ~TestState() noexcept {}

    virtual void add_field_handlers() {}
    virtual void add_indirect_field_handlers() {}

    virtual int32_t threads() const noexcept {
        return 1;
    }

    template <typename T>
    void add_field_handler(const char* name, T& field)
    {
        handlers_.push_back(std::make_unique<TypedFieldHandler<T>>(name, field));
    }

    template <typename T>
    void add_indirect_field_handler(const char* name, T& field)
    {
        handlers_.push_back(std::make_unique<TypedIndirectFieldHandler<T>>(name, field));
    }

    void externalize(YAML::Node& node, ConfigurationContext* context)
    {
        for (auto& handler: handlers_)
        {
            handler->externalize(node, context);
        }
    }

    void internalize(const YAML::Node& node, ConfigurationContext* context)
    {
        for (auto& handler: handlers_)
        {
            handler->internalize(node, context);
        }
    }

    virtual void pre_configure(TestCoverage coverage)  {}
    virtual void post_configure(TestCoverage coverage) {}
};

class CommonConfigurationContext: public ConfigurationContext {

    filesystem::path config_base_path_;

public:
    CommonConfigurationContext(filesystem::path config_base_path):
        config_base_path_(config_base_path)
    {}

    virtual filesystem::path resource_path(const std::string& name)
    {
        filesystem::path pp = config_base_path_;
        return pp.append(name.data());
    }
};


#define MMA1_TESTS_STATE_FILED(name) this->add_field_handler(MMA1_TOSTRING(name), name)
#define MMA1_TESTS_INDIRECT_STATE_FILED(name) this->add_indirect_field_handler(MMA1_TOSTRING(name), name)

#define MMA1_BOOST_PP_STATE_FILED(r, data, elem) MMA1_TESTS_STATE_FILED(elem);\

#define MMA1_BOOST_PP_INDIRECT_STATE_FILED(r, data, elem) MMA1_TESTS_INDIRECT_STATE_FILED(elem);\


#define MMA1_STATE_FILEDS(...)          \
    virtual void add_field_handlers() { \
        Base::add_field_handlers();     \
        BOOST_PP_LIST_FOR_EACH(MMA1_BOOST_PP_STATE_FILED, _, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)) \
    }


#define MMA1_INDIRECT_STATE_FILEDS(...)             \
    virtual void add_indirect_field_handlers() {    \
        Base::add_indirect_field_handlers();        \
        BOOST_PP_LIST_FOR_EACH(MMA1_BOOST_PP_INDIRECT_STATE_FILED, _, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)) \
    }


}}}
