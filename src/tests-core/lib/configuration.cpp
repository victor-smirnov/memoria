
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





#include <memoria/v1/tests/configuration.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>

#include <boost/filesystem.hpp>

#include <map>
#include <fstream>
#include <memory>
#include <iostream>

namespace memoria {
namespace v1 {

namespace bf = boost::filesystem;
    
using namespace std;

StringList::StringList(U16StringRef list, U16StringRef separators)
{
    size_t pos = 0;
    while (pos != U16String::NPOS && pos < list.length())
    {
        size_t idx = list.find_first_of(separators.data(), pos);
        if (idx != U16String::NPOS)
        {
            list_.push_back(list.substring(pos, idx - pos).trim());
            pos = idx + 1;
        }
        else {
            list_.push_back(list.substring(pos, list.length() - pos).trim());
            pos = String::npos;
        }
    }
}

int StringList::size() const
{
    return list_.size();
}

U16StringRef StringList::getItem(int32_t idx) const
{
    if (idx >=0 && idx < (int32_t)list_.size())
    {
        return list_[idx];
    }
    else {
        throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Index is out of bounds in a StringList: "
                <<idx<<" max="
                <<list_.size());
    }
}

Configurator::Configurator(Configurator *parent): parent_(parent) {}

Configurator::~Configurator() throw () {}

Configurator* Configurator::getParent() const {
    return parent_;
}

void Configurator::setParent(Configurator* parent) {
    parent_ = parent;
}

void Configurator::AddProperty(U16StringRef name, U16StringRef value)
{
    properties_[name] = value;
}

void Configurator::removeProperty(U16StringRef name) {
    properties_.erase(name);
}

U16String Configurator::resolve_references(U16StringRef value, NameTree* names) const
{
    stringstream buf;

    uint32_t pos = 0;
    while (pos < value.length())
    {
        size_t idx1 = value.find(u"${", pos);
        if (idx1 != U16String::NPOS)
        {
            buf << value.substring(pos, idx1 - pos);
            size_t idx2 = value.find(u"}", idx1 + 2);

            if (idx2 != U16String::NPOS)
            {
                U16String name = value.substring(idx1 + 2, idx2 - idx1 - 2);
                if(!names->find(name))
                {
                    NameTree names0(&name, names);
                    buf<<this->get_property(name, &names0, true);
                }
                else {
                    throw Exception(MEMORIA_SOURCE, SBuf() << "Circular property reference: " << name);
                }
            }
            else {
                throw Exception(MEMORIA_SOURCE, SBuf() << "Invalid property reference format: " << value);
            }
            pos = idx2 + 1;
        }
        else {
            break;
        }
    }

    if (pos < value.length())
    {
        buf<<value.substring(pos, value.length() - pos);
    }

    return U8String(buf.str()).to_u16();
}

U16String Configurator::get_property(U16StringRef name, NameTree* names, bool resolve) const
{
    auto i = properties_.find(name);
    if (i != properties_.end())
    {
        U16StringRef value = i->second;
        if (value.find(u"${", 0) != U16String::NPOS && resolve)
        {
            return resolve_references(value, names);
        }
        else {
            return value; //no property references in the value
        }
    }
    else if (parent_ != NULL) {
        return parent_->get_property(name, names, resolve);
    }
    else {
        return "";
    }
}

U16String Configurator::getProperty(U16StringRef name, bool resolve) const
{
    NameTree names(&name);
    return get_property(name, &names, resolve);
}



bool Configurator::IsPropertyDefined(U16StringRef name) const
{
    StringMapType::const_iterator i = properties_.find(name);
    if (i != properties_.end()) {
        return true;
    }
    else if (parent_ != NULL) {
        return parent_->IsPropertyDefined(name);
    }
    else {
        return false;
    }
}


const Configurator::StringMapType& Configurator::getThisConfigurationProperties() const
{
    return properties_;
}

Configurator::PropertyListType* Configurator::getPropertyList() const
{
    const Configurator* cfg = this;

    Configurator::PropertyListType* list = new Configurator::PropertyListType();

    while (cfg != NULL)
    {
        for (Configurator::StringMapType::const_iterator i = cfg->properties_.begin(); i != properties_.end(); i++)
        {
            list->insert(i->first);
        }
        cfg = cfg->parent_;
    }

    return list;
}

void add_line(Configurator *cfg, StringRef str, size_t start, size_t end, StringRef sep)
{
    typedef size_t SizeT;

    if (start != U16String::NPOS && start < str.length() && str[start] != '#' && !isEmpty(str, start, end, sep))
    {
        if (end == U16String::NPOS) end = str.length();

        SizeT divider = str.find("=", start);
        if (divider != U16String::NPOS)
        {
            String name = trimString(str.substr(start, divider - start));

            //SizeT pos = divider + 1;

            if (divider + 1 < str.length())
            {
                cfg->AddProperty(U16String(U8String(name)), U16String(U8String(trimString(str.substr(divider + 1, end - divider - 1)))));
            }
            else {
                cfg->AddProperty(U16String(U8String(name)), u"");
            }
        }
        else {
            throw Exception(MEMORIA_SOURCE, SBuf() << "There is no '=' in the string '"
                                                   << trimString(str.substr(start, end - start))
                                                   << "'");
        }
    }
    else {
        // ignore comments and empty lines
    }
}



bool get_line(StringRef text, size_t pos, size_t& nl_start, StringRef sep)
{
    if (pos < text.length() && pos != String::npos) {
        nl_start = text.find(sep, pos);
        return true;
    }
    else {
        return false;
    }
}

Configurator* Configurator::Parse(U16StringRef file_name, Configurator* cfg)
{
    ifstream file;
    file.open(file_name.to_u8().data());
    if (file.is_open())
    {
        stringbuf buf;

        if(cfg == NULL) cfg = new Configurator();

        file >> &buf;

        String text = buf.str();
        typedef size_t SizeT;

        String sep = Platform::getLineSeparator().to_u8().to_std_string();

        SizeT pos = text.find_first_not_of(sep + "\t ");

        if (pos != String::npos)
        {
            SizeT nl_start;

            stringbuf* buf = NULL;

            while (get_line(text, pos, nl_start, sep))
            {
                if (buf == NULL)
                {
                    if (nl_start != String::npos)
                    {
                        if (nl_start > pos && text[nl_start - 1] == '\\')
                        {
                            buf = new stringbuf();
                            buf->sputn(text.data() + pos, nl_start - pos - 1);
                        }
                        else {
                            add_line(cfg, text, pos, nl_start, sep);
                        }
                    }
                    else {
                        SizeT e;
                        if (text[text.length() - 1] == '\\') e = text.length() - 1;
                        else e = nl_start;

                        add_line(cfg, text, pos, e, sep);
                        break;
                    }
                }
                else {
                    if (isEmpty(text, pos, nl_start, sep))
                    {
                        String line = buf->str();
                        add_line(cfg, line, 0, String::npos, sep);
                        delete buf;
                        buf = NULL;
                    }
                    else {
                        SizeT idx = text.find_first_not_of("\t ", pos);
                        if (idx == pos)
                        {
                            String line = buf->str();
                            add_line(cfg, line, 0, String::npos, sep);
                            delete buf;

                            if (text[nl_start - 1] == '\\') {
                                buf = new stringbuf();
                                buf->sputn(text.data() + pos, nl_start - pos - 1);
                            }
                            else {
                                add_line(cfg, text, pos, nl_start, sep);
                                buf = NULL;
                            }
                        }
                        else {
                            SizeT e = nl_start;
                            if (text[nl_start - 1] == '\\') e--;
                            buf->sputn(text.data() + idx, e - idx);
                        }
                    }
                }

                pos = (nl_start != String::npos ? nl_start : text.length()) + sep.length();
            }
        }

        file.close();
        return cfg;
    }
    else {
        throw Exception(MEMORIA_SOURCE, SBuf()<<"Can't open file for reading: "<<file_name);
    }
}

Configurator* Configurator::BuildRootConfigurator(const char** envp) {
    Configurator* cfg = new Configurator();

    if (envp != NULL)
    {
        //add environment variables into configurator in the form
        //env.NAME => VALUE
        for (; NULL != *envp; envp++)
        {
            U16String pair(*envp);
            size_t idx = pair.find(u"=", 0);
            if (idx != U16String::NPOS)
            {
                U16String name = U16String(u"env.") + pair.substring(0, idx).trim();
                if (idx < pair.length() - 1)
                {
                    U16String value = pair.substring(idx + 1, pair.length()).trim();
                    cfg->AddProperty(name, value);
                }
                else {
                    cfg->AddProperty(name, u"");
                }
            }
        }
    }

    cfg->AddProperty(u"sys.arch.x86_64",             u"x86_64");
    cfg->AddProperty(u"sys.arch.i386",               u"i386");
    cfg->AddProperty(u"sys.arch.arm32",              u"arm32");

    cfg->AddProperty(u"memoria.dir",                 u"memoria");
    cfg->AddProperty(u"memoria.local.dir",           u".${memoria.dir}");

    return cfg;
}


Configurator* Configurator::BuildChain(const char** envp, bool read_config_files)
{
    Configurator* root = BuildRootConfigurator(envp);
    Configurator* platform = root;

    if (read_config_files)
    {
        PathList list(platform->getProperty(u"config.search.path"));

        for (int32_t c = 0; c < list.size(); c++)
        {
            try {
                String dirPath = list.getItem(c).to_u8().to_std_string();
                
                if (bf::exists(dirPath))
                {
                    if (!bf::is_directory(dirPath))
                    {
                        Configurator* cfg = Configurator::Parse(U16String(dirPath));
                        cfg->setParent(platform);
                        platform = cfg;
                    }
                    else {
                        Configurator* cfg = NULL;

                        for (auto& entry : bf::directory_iterator(dirPath)) 
                        {
                            if (bf::is_directory(entry.path()))
                            {
                                if (isEndsWith(entry.path().string(), ".props"))
                                {
                                    cfg = Configurator::Parse(U16String(entry.path().string()), cfg);
                                }
                            }
                        }
                        
                        if (cfg)
                        {
                            cfg->setParent(platform);
                            platform = cfg;
                        }
                    }
                }
            }
            catch (MemoriaThrowable e)
            {
                cout << e.source() << ": " << e <<endl;
            }
        }
    }

    return platform;
}





}}
