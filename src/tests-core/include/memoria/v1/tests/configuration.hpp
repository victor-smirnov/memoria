
// Copyright 2012 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/platform.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/strings/strings.hpp>
#include <memoria/v1/core/tools/strings/format.hpp>

#include <map>
#include <set>
#include <vector>

namespace memoria {
namespace v1 {

class ListOfStrings;
class Configurator;


class Configurator {
public:

        typedef std::map<U16String, U16String>             StringMapType;
        typedef std::set<U16String>                        PropertyListType;

        Configurator(Configurator *parent = NULL);

        virtual ~Configurator() throw ();

        virtual Configurator* getParent() const;
        virtual void setParent(Configurator* parent);

        virtual void AddProperty(U16StringRef name, U16StringRef value);
        virtual void removeProperty(U16StringRef name);

        virtual bool IsPropertyDefined(U16StringRef name) const;
        virtual U16String getProperty(U16StringRef name, bool resolve = true) const;

        virtual const StringMapType& getThisConfigurationProperties() const;

        virtual PropertyListType* getPropertyList() const;

        static Configurator* Parse(U16StringRef fileName, Configurator* cfg = NULL);

        static Configurator* BuildChain(const char** envp, bool read_config_files = true);

        template <typename T>
        T getValue(U16StringRef name) const
        {
            if (this->IsPropertyDefined(name))
            {
                return FromString<T>::convert(this->getProperty(name));
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Property {} is not specified", name));
            }
        }

        template <typename T>
        T getValue(U16StringRef name, const T& default_value) const
        {
            if (this->IsPropertyDefined(name))
            {
                return FromString<T>::convert(this->getProperty(name));
            }
            else
            {
                return default_value;
            }
        }

protected:

        static Configurator* BuildRootConfigurator(const char** envp);
        static Configurator* BuildPlatformDefaultsConfigurator();

        class NameTree {
                const U16String* name_;
                NameTree* parent_;
        public:
                NameTree(const U16String* name, NameTree* parent = NULL):
                        name_(name),
                        parent_(parent)
                {}

                bool find(U16StringRef name) const
                {
                    if (name == *name_)
                    {
                        return true;
                    }
                    else if (parent_ != NULL)
                    {
                        return parent_->find(name);
                    }
                    else
                    {
                        return false;
                    }
                }
        };

        virtual U16String resolve_references(U16StringRef value, NameTree* names) const;
        virtual U16String get_property(U16StringRef name, NameTree *names, bool resolve) const;

private:
        Configurator* parent_;
        StringMapType properties_;
};

class StringList {
        std::vector<U16String> list_;
public:
        StringList(U16StringRef list, U16StringRef separators = u",");

        int size() const;
        U16StringRef getItem(int32_t size) const;
};

class PathList: public StringList {
public:
        PathList(U16StringRef list, U16StringRef separator = Platform::getPathSeparator()): StringList(list, separator) {};
};



}}
