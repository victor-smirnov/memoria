
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

#include <memoria/core/types.hpp>
#include <boost/filesystem/path.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/random.hpp>

#include <memoria/tests/serialization.hpp>


#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>

#include <yaml-cpp/yaml.h>

#include <string>
#include <iostream>

namespace memoria {
namespace tests {



template <typename T> struct IndirectStateFiledSerializer;

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
        node[name_] = path.string();
        IndirectStateFiledSerializer<T>::externalize(field_, path, context);
    }

    virtual void internalize(const YAML::Node& node, ConfigurationContext* context)
    {
        if (node[name_])
        {
            auto file_name = node[name_].template as<std::string>();
            boost::filesystem::path file_path = file_name;

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
    bool replay_;
    int64_t seed_;
public:
    boost::filesystem::path working_directory_;

    TestState() = default;
    TestState(TestState&&) = default;
    TestState(const TestState&) = delete;

    virtual ~TestState() noexcept;

    virtual void add_field_handlers() {
        add_field_handler("seed", seed_);
    }
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

    int32_t getRandom() const {
        return getRandomG();
    }

    int32_t getRandom(int32_t max) const {
        return getRandomG(max);
    }

    int32_t getRandom1(int32_t max) const {
        return getRandomG(max - 1) + 1;
    }

    int64_t getBIRandom() const {
        return getBIRandomG();
    }

    int64_t getBIRandom(int64_t max) const {
        return getBIRandomG(max);
    }

    std::ostream& out() const;

    template <typename... Args>
    std::ostream& println(const char* fmt, Args&&... args) const
    {
        out() << format_u8(fmt, std::forward<Args>(args)...) << std::endl;
        return out();
    }

    virtual void set_up() noexcept {}
    virtual void tear_down() noexcept {}

    virtual void on_test_failure() noexcept {}

    bool is_replay() const noexcept {
        return replay_;
    }

    void set_replay(bool replay) {
        replay_ = replay;
    }

    int64_t seed() const noexcept {
        return seed_;
    }

    void set_seed(int64_t seed) {
        seed_ = seed;
    }
};

class CommonConfigurationContext: public ConfigurationContext {

    boost::filesystem::path config_base_path_;

public:
    CommonConfigurationContext(boost::filesystem::path config_base_path):
        config_base_path_(config_base_path)
    {}

    virtual boost::filesystem::path resource_path(const std::string& name)
    {
        boost::filesystem::path pp = config_base_path_;
        return pp.append(name.data());
    }
};


#define MMA_TESTS_STATE_FILED(name) this->add_field_handler(MMA_TOSTRING(name), name)
#define MMA_TESTS_INDIRECT_STATE_FILED(name) this->add_indirect_field_handler(MMA_TOSTRING(name), name)

#define MMA_BOOST_PP_STATE_FILED(r, data, elem) MMA_TESTS_STATE_FILED(elem);\

#define MMA_BOOST_PP_INDIRECT_STATE_FILED(r, data, elem) MMA_TESTS_INDIRECT_STATE_FILED(elem);\


#define MMA_STATE_FILEDS(...)          \
    virtual void add_field_handlers() { \
        Base::add_field_handlers();     \
        BOOST_PP_LIST_FOR_EACH(MMA_BOOST_PP_STATE_FILED, _, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)) \
    }


#define MMA_INDIRECT_STATE_FILEDS(...)             \
    virtual void add_indirect_field_handlers() {    \
        Base::add_indirect_field_handlers();        \
        BOOST_PP_LIST_FOR_EACH(MMA_BOOST_PP_INDIRECT_STATE_FILED, _, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__)) \
    }


}}
