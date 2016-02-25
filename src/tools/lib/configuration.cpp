
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




#include <memoria/tools/configuration.hpp>
#include <memoria/core/tools/file.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings/strings.hpp>

#include <map>
#include <fstream>
#include <memory>
#include <iostream>

namespace memoria {

using namespace std;

StringList::StringList(StringRef list, StringRef separators)
{
    String::size_type pos = 0;
    while (pos != String::npos && pos < list.length())
    {
        String::size_type idx = list.find_first_of(separators, pos);
        if (idx != String::npos)
        {
            list_.push_back(trimString(list.substr(pos, idx - pos)));
            pos = idx + 1;
        }
        else {
            list_.push_back(trimString(list.substr(pos, list.length() - pos)));
            pos = String::npos;
        }
    }
}

int StringList::size() const
{
    return list_.size();
}

StringRef StringList::getItem(Int idx) const
{
    if (idx >=0 && idx < (Int)list_.size())
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

void Configurator::AddProperty(StringRef name, StringRef value)
{
    properties_[name] = value;
}

void Configurator::removeProperty(StringRef name) {
    properties_.erase(name);
}

String Configurator::resolve_references(StringRef value, NameTree* names) const
{
    stringstream buf;

    UInt pos = 0;
    while (pos < value.length())
    {
        size_t idx1 = value.find("${", pos);
        if (idx1 != String::npos)
        {
            buf<<value.substr(pos, idx1 - pos);
            size_t idx2 = value.find("}", idx1 + 2);

            if (idx2 != String::npos)
            {
                String name = value.substr(idx1 + 2, idx2 - idx1 - 2);
                if(!names->find(name))
                {
                    NameTree names0(&name, names);
                    buf<<this->get_property(name, &names0, true);
                }
                else {
                    throw Exception(MEMORIA_SOURCE, SBuf()<<"Circular property reference: "<<name);
                }
            }
            else {
                throw Exception(MEMORIA_SOURCE, SBuf()<<"Invalid property reference format: "<<value);
            }
            pos = idx2 + 1;
        }
        else {
            break;
        }
    }

    if (pos < value.length())
    {
        buf<<value.substr(pos, value.length() - pos);
    }

    return buf.str();
}

String Configurator::get_property(StringRef name, NameTree* names, bool resolve) const
{
    auto i = properties_.find(name);
    if (i != properties_.end())
    {
        StringRef value = i->second;
        if (value.find("${", 0) != String::npos && resolve)
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

String Configurator::getProperty(StringRef name, bool resolve) const
{
    NameTree names(&name);
    return get_property(name, &names, resolve);
}



bool Configurator::IsPropertyDefined(StringRef name) const
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

void add_line(Configurator *cfg, StringRef str, String::size_type start, String::size_type end, StringRef sep)
{
    typedef String::size_type SizeT;

    if (start != String::npos && start < str.length() && str[start] != '#' && !isEmpty(str, start, end, sep))
    {
        if (end == String::npos) end = str.length();

        SizeT divider = str.find("=", start);
        if (divider != String::npos)
        {
            String name = trimString(str.substr(start, divider - start));

            //SizeT pos = divider + 1;

            if (divider + 1 < str.length())
            {
                cfg->AddProperty(name, trimString(str.substr(divider + 1, end - divider - 1) ));
            }
            else {
                cfg->AddProperty(name, "");
            }
        }
        else {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"There is no '=' in the string '"
                                                  <<trimString(str.substr(start, end - start))
                                                  <<"'");
        }
    }
    else {
        // ignore comments and empty lines
    }
}



bool get_line(StringRef text, String::size_type pos, String::size_type& nl_start, StringRef sep)
{
    if (pos < text.length() && pos != String::npos) {
        nl_start = text.find(sep, pos);
        return true;
    }
    else {
        return false;
    }
}

Configurator* Configurator::Parse(StringRef file_name, Configurator* cfg)
{
    ifstream file;
    file.open(file_name.c_str());
    if (file.is_open())
    {
        stringbuf buf;

        if(cfg == NULL) cfg = new Configurator();

        file>>&buf;

        String text = buf.str();
        typedef String::size_type SizeT;

        String sep = Platform::getLineSeparator();

        SizeT pos = text.find_first_not_of(sep+"\t ");

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
            String pair(*envp);
            size_t idx = pair.find("=", 0);
            if (idx != String::npos) {
                String name = "env." + trimString(pair.substr(0, idx));
                if (idx < pair.length() - 1) {
                    String value = trimString(pair.substr(idx + 1, pair.length()));
                    cfg->AddProperty(name, value);
                }
                else {
                    cfg->AddProperty(name, "");
                }
            }
        }
    }

    cfg->AddProperty("sys.arch.x86_64",             "x86_64");
    cfg->AddProperty("sys.arch.i386",               "i386");
    cfg->AddProperty("sys.arch.arm32",              "arm32");

    cfg->AddProperty("memoria.dir",                 "memoria");
    cfg->AddProperty("memoria.local.dir",           ".${memoria.dir}");

    return cfg;
}


Configurator* Configurator::BuildChain(const char** envp, bool read_config_files)
{
    Configurator* root = BuildRootConfigurator(envp);
    Configurator* platform = root;//BuildPlatformDefaultsConfigurator();
    //        platform->setParent(root);

    if (read_config_files)
    {
        PathList list(platform->getProperty("config.search.path"));

        for (Int c = 0; c < list.size(); c++)
        {
            try {
                String dirPath = list.getItem(c);
                File path(dirPath);
                if (path.isExists())
                {
                    if (!path.isDirectory())
                    {
                        Configurator* cfg = Configurator::Parse(path.getPath());
                        cfg->setParent(platform);
                        platform = cfg;
                    }
                    else {
                        unique_ptr<File::FileListType> list(File::readDir(path));
                        Configurator* cfg = NULL;

                        for (File::FileListType::iterator i = list->begin(); i != list->end(); i++)
                        {
                            File* file = *i;
                            if (!file->isDirectory())
                            {
                                if (isEndsWith(file->getName(), ".props"))
                                {
                                    cfg = Configurator::Parse(file->getPath(), cfg);
                                }
                            }
                        }

                        if (cfg != NULL)
                        {
                            cfg->setParent(platform);
                            platform = cfg;
                        }
                    }
                }
            }
            catch (MemoriaThrowable e)
            {
                cout<<e.source()<<": "<<e<<endl;
            }
        }
    }

    return platform;
}





}


