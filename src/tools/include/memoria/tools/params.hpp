
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_PARAMS_HPP
#define	_MEMORIA_TOOLS_PARAMS_HPP

#include <limits>
#include <stdlib.h>

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/tools/strings.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/tools/configuration.hpp>



namespace memoria {

using namespace std;
using namespace memoria::vapi;





class AbstractParamDescriptor {
public:
	virtual void Process(Configurator* cfg) 	= 0;
	virtual StringRef GetName() const			= 0;
	virtual String GetPropertyName() const		= 0;
	virtual void Dump(std::ostream& os) const	= 0;
};


template <typename T> class ParamDescriptor;

class ParametersSet {

	String 			prefix_;

	vector<AbstractParamDescriptor*> descriptors_;

public:
	ParametersSet(StringRef prefix): prefix_(prefix) {}
	ParametersSet(const ParametersSet&) = delete;

	StringRef GetPrefix() const
	{
		return prefix_;
	}

	void SetPrefix(StringRef prefix)
	{
		prefix_ = prefix;
	}

	virtual AbstractParamDescriptor* Put(AbstractParamDescriptor* descr);

	template <typename T>
	ParamDescriptor<T>* Add(StringRef name, T& property)
	{
		return T2T_S<ParamDescriptor<T>*>(Put(new ParamDescriptor<T>(this, name, property)));
	}

	template <typename T>
	ParamDescriptor<T>* Add(StringRef name, T& property, const T& max_value)
	{
		return T2T_S<ParamDescriptor<T>*>(Put(new ParamDescriptor<T>(this, name, property, max_value)));
	}

	template <typename T>
	ParamDescriptor<T>* Add(StringRef name, T& property, const T& min_value, const T& max_value)
	{
		return T2T_S<ParamDescriptor<T>*>(Put(new ParamDescriptor<T>(this, name, property, min_value, max_value)));
	}

	void DumpProperties(std::ostream& os) const;

	void Process(Configurator* cfg);
};


template <typename T>
class ParamDescriptor: public AbstractParamDescriptor {

	typedef ParamDescriptor<T> 						MyType;

	ParametersSet* 		cfg_;

	String 				name_;

	T& 					value_;

	T 					min_value_;
	T 					max_value_;

	bool				ranges_specified_;

	bool				mandatory_;

public:

	ParamDescriptor(ParametersSet* cfg, String name, T& value):
		cfg_(cfg),
		name_(name),
		value_(value),
		ranges_specified_(false),
		mandatory_(false)
	{}

	ParamDescriptor(ParametersSet* cfg, String name, T& value, const T& max_value):
		cfg_(cfg),
		name_(name),
		value_(value),
		min_value_(numeric_limits<T>::min()),
		max_value_(max_value),
		ranges_specified_(true),
		mandatory_(false)
	{}

	ParamDescriptor(ParametersSet* cfg, String name, T& value, const T& min_value, const T& max_value):
		cfg_(cfg),
		name_(name),
		value_(value),
		min_value_(min_value),
		max_value_(max_value),
		ranges_specified_(true),
		mandatory_(false)
	{}


	virtual void Process(Configurator* cfg)
	{
		SetValue(cfg, value_);

		if (ranges_specified_)
		{
			if (!(value_ >= min_value_ && value_ <= max_value_))
			{
				throw Exception(MEMORIA_SOURCE, SBuf()<<"Range checking failure for the property: "<<prefix()<<"."<<name_);
			}
		}
	}

	virtual bool IsMandatory() const
	{
		return mandatory_;
	}

	MyType* SetMandatory(bool mandatory)
	{
		mandatory_ = mandatory;
		return this;
	}

	virtual StringRef GetName() const
	{
		return name_;
	}

	virtual String GetPropertyName() const
	{
		if (IsEmpty(prefix()))
		{
			return name_;
		}
		else {
			return prefix()+"."+name_;
		}
	}

	virtual void Dump(std::ostream& os) const
	{
		bool doc = ranges_specified_;

		if (doc)
		{
			os<<"#";
		}

		//FIXME: print default value in property dump
		//os<<"default: "<<AsString<T>::convert(default_value_)<<" ";

		if (ranges_specified_)
		{
			//FIXME: Is conversion to string is necessary here?
			os<<"Range from: "<<AsString<T>::convert(min_value_)<<" to "<<AsString<T>::convert(max_value_);
		}

		if (doc)
		{
			os<<endl;
		}

		os<<GetPropertyName()<<"="<<AsString<T>::convert(value_)<<endl;

		if (doc)
		{
			os<<endl;
		}
	}

	StringRef prefix() const
	{
		return cfg_->GetPrefix();
	}

protected:
	void SetValue(Configurator* cfg, T& value);
};





template <typename T>
void ParamDescriptor<T>::SetValue(Configurator* cfg, T& value)
{
	StringRef prefix1 = prefix();

	auto pos = prefix1.length();

	while (true)
	{
		String name = prefix().substr(0, pos) + "." + name_;

		if (cfg->IsPropertyDefined(name))
		{
			value = FromString<T>::convert(cfg->GetProperty(name));
			return;
		}
		else {
			pos = prefix().find_last_of(".", pos - 1);

			if (pos == String::npos)
			{
				break;
			}
		}
	}

	if (cfg->IsPropertyDefined(name_))
	{
		value = FromString<T>::convert(cfg->GetProperty(name_));
		return;
	}
	else if (mandatory_)
	{
		throw Exception(MEMORIA_SOURCE, SBuf()<<"Property "<<name_<<" is not defined in the program configuration");
	}
}



}


#endif
