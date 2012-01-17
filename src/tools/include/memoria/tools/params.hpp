
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_PARAMS_HPP
#define	_MEMORIA_TOOLS_PARAMS_HPP

#include <limits>
#include <stdlib.h>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/strings.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/tools/configuration.hpp>



namespace memoria {

using namespace std;
using namespace memoria::vapi;





class AbstractParamDescriptor {
public:
	virtual void Process(Configurator* cfg) 	= 0;
	virtual String GetPropertyName() 			= 0;
	virtual void Dump(std::ostream& os) 		= 0;
};


class ParametersSet;

template <typename T>
class ParamDescriptor: public AbstractParamDescriptor {
	ParametersSet* 		cfg_;

	String 				prefix_;
	String 				name_;

	T& 					value_;

	T					default_value_;
	T 					min_value_;
	T 					max_value_;

	bool				default_value_specified_;
	bool				ranges_specified_;
public:
	ParamDescriptor(ParametersSet* cfg, StringRef prefix, String name, T& value):
		cfg_(cfg),
		prefix_(prefix),
		name_(name),
		value_(value),
		default_value_specified_(false),
		ranges_specified_(false)
	{}

	ParamDescriptor(ParametersSet* cfg, StringRef prefix, String name, T& value, const T& default_value):
		cfg_(cfg),
		prefix_(prefix),
		name_(name),
		value_(value),
		default_value_(default_value),
		default_value_specified_(true),
		ranges_specified_(false)
	{}

	ParamDescriptor(ParametersSet* cfg, StringRef prefix, String name, T& value, const T& default_value, const T& max_value):
		cfg_(cfg),
		prefix_(prefix),
		name_(name),
		value_(value),
		default_value_(default_value),
		min_value_(numeric_limits<T>::min()),
		max_value_(max_value),
		default_value_specified_(true),
		ranges_specified_(true)
	{}

	ParamDescriptor(ParametersSet* cfg, StringRef prefix, String name, T& value, const T& default_value, const T& min_value, const T& max_value):
		cfg_(cfg),
		prefix_(prefix),
		name_(name),
		value_(value),
		default_value_(default_value),
		min_value_(min_value),
		max_value_(max_value),
		default_value_specified_(true),
		ranges_specified_(true)
	{}


	virtual void Process(Configurator* cfg)
	{
		value_ = GetValue(cfg);

		if (ranges_specified_)
		{
			if (!(value_ >= min_value_ && value_ <= max_value_))
			{
				throw MemoriaException(MEMORIA_SOURCE, "Range checking failure for the property: "+prefix_+"."+name_);
			}
		}
	}

	virtual String GetPropertyName()
	{
		if (IsEmpty(prefix_))
		{
			return name_;
		}
		else {
			return prefix_+"."+name_;
		}
	}

	virtual void Dump(std::ostream& os)
	{
		bool doc = default_value_specified_ || ranges_specified_;

		if (doc)
		{
			os<<"#";
		}

		if (default_value_specified_)
		{
			os<<"default: "<<AsString<T>::convert(default_value_)<<" ";
		}

		if (ranges_specified_)
		{
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

protected:
	T GetValue(Configurator* cfg);
};


class ParametersSet {

	String 			prefix_;

	vector<AbstractParamDescriptor*> descriptors_;

public:
	ParametersSet(StringRef prefix): prefix_(prefix) {}

	StringRef GetPrefix() const
	{
		return prefix_;
	}

	template <typename T>
	void Add(StringRef name, T& property)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, prefix_, name, property));
	}

	template <typename T>
	void Add(StringRef name, T& property, const T& default_value)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, prefix_, name, property, default_value));
	}

	template <typename T>
	void Add(StringRef name, T& property, const T& default_value, const T& max_value)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, prefix_, name, property, default_value, max_value));
	}

	template <typename T>
	void Add(StringRef name, T& property, const T& default_value, const T& min_value, const T& max_value)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, prefix_, name, property, default_value, min_value, max_value));
	}

	void DumpProperties(std::ostream& os);

	void Process(Configurator* cfg);
};



template <typename T>
T ParamDescriptor<T>::GetValue(Configurator* cfg)
{
	String ext_name = GetPropertyName();
	if (cfg->IsPropertyDefined(ext_name))
	{
		return FromString<T>::convert(cfg->GetProperty(ext_name));
	}
	else if (cfg->IsPropertyDefined(name_))
	{
		return FromString<T>::convert(cfg->GetProperty(name_));
	}
	else if (default_value_specified_)
	{
		return default_value_;
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "Property "+ext_name+" has to be specified in the config file");
	}
}



}


#endif
