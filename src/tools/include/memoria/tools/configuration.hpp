/*
 * configuration.hpp
 *
 *  Created on: 08.01.2012
 *      Author: developer
 */

#ifndef MEMORIA_TOOLS_CONFIGURATION_HPP_
#define MEMORIA_TOOLS_CONFIGURATION_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/platform.hpp>

#include <map>
#include <set>
#include <vector>

namespace memoria {

class ListOfStrings;
class Configurator;


class Configurator {
public:

        typedef std::map<String, String>        		StringMapType;
        typedef std::set<String>                        PropertyListType;

        Configurator(Configurator *parent = NULL);
        virtual ~Configurator() throw ();

        virtual Configurator* GetParent() const;
        virtual void SetParent(Configurator* parent);

        virtual void AddProperty(StringRef name, StringRef value);
        virtual void RemoveProperty(StringRef name);

        virtual bool IsPropertyDefined(StringRef name) const;
        virtual String GetProperty(StringRef name, bool resolve = true) const;

        virtual const StringMapType& GetThisConfigurationProperties() const;

        virtual PropertyListType* GetPropertyList() const;

        static Configurator* Parse(StringRef fileName, Configurator* cfg = NULL);

        static Configurator* BuildChain(const char** envp, bool read_config_files = true);
protected:

        static Configurator* BuildRootConfigurator(const char** envp);
        static Configurator* BuildPlatformDefaultsConfigurator();

        class NameTree {
                const String* name_;
                NameTree* parent_;
        public:
                NameTree(const String* name, NameTree* parent = NULL):
                        name_(name),
                        parent_(parent)
                {}

                bool find(StringRef name) const {
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

        virtual String resolve_references(StringRef value, NameTree* names) const;
        virtual String get_property(StringRef name, NameTree *names, bool resolve) const;

private:
        Configurator* parent_;
        StringMapType properties_;
};

class StringList {
        std::vector<String> list_;
public:
        StringList(StringRef list, StringRef separators = ",");

        int Size() const;
        StringRef GetItem(Int size) const;
};

class PathList: public StringList {
public:
        PathList(StringRef list, StringRef separator = Platform::GetPathSeparator()): StringList(list, separator) {};
};



}


#endif
