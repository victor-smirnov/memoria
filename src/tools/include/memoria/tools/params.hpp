
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_PARAMS_HPP
#define	_MEMORIA_TOOLS_PARAMS_HPP

#include <limits>
#include <stdlib.h>

#include <memoria/core/types/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/tools/configuration.hpp>


namespace memoria {

using namespace std;
using namespace memoria::vapi;

long int ConvertToLongInt(StringRef str);
long long ConvertToLongLong(StringRef str);
double ConvertToDouble(StringRef str);
long double ConvertToLongDouble(StringRef str);

template <typename T> struct FromString;

template <>
struct FromString<Int> {
	static Int convert(StringRef str)
	{
		return ConvertToLongInt(str);
	}
};

template <>
struct FromString<BigInt> {
	static BigInt convert(StringRef str)
	{
		return ConvertToLongLong(str);
	}
};

template <>
struct FromString<double> {
	static double convert(StringRef str)
	{
		return ConvertToDouble(str);
	}
};

template <>
struct FromString<bool> {
	static bool convert(StringRef str)
	{
		return ConvertToLongInt(str);
	}
};


class AbstractParamDescriptor {
public:
	virtual void Process() 				= 0;
	virtual String GetPropertyName() 	= 0;
	virtual void Dump(std::ostream& os) = 0;
};


class TaskParametersSet;

template <typename T>
class ParamDescriptor: public AbstractParamDescriptor {
	TaskParametersSet* 	cfg_;
	StringRef 			set_name_;
	String 			name_;

	T& 				value_;

	T				default_value_;
	T 				min_value_;
	T 				max_value_;

	bool			default_value_specified_;
	bool			ranges_specified_;
public:
	ParamDescriptor(TaskParametersSet* cfg, StringRef set_name, String name, T& value):
		cfg_(cfg),
		set_name_(set_name),
		name_(name),
		value_(value),
		default_value_specified_(false),
		ranges_specified_(false)
	{}

	ParamDescriptor(TaskParametersSet* cfg, StringRef set_name, String name, T& value, const T& default_value):
		cfg_(cfg),
		set_name_(set_name),
		name_(name),
		value_(value),
		default_value_(default_value),
		ranges_specified_(false)
	{}

	ParamDescriptor(TaskParametersSet* cfg, StringRef set_name, String name, T& value, const T& default_value, const T& max_value):
		cfg_(cfg),
		set_name_(set_name),
		name_(name),
		value_(value),
		default_value_(default_value),
		min_value_(numeric_limits<T>::min()),
		max_value_(max_value),
		default_value_specified_(true),
		ranges_specified_(true)
	{}

	ParamDescriptor(TaskParametersSet* cfg, StringRef set_name, String name, T& value, const T& default_value, const T& min_value, const T& max_value):
		cfg_(cfg),
		set_name_(set_name),
		name_(name),
		value_(value),
		default_value_(default_value),
		min_value_(min_value),
		max_value_(max_value),
		default_value_specified_(true),
		ranges_specified_(true)
	{}


	virtual void Process()
	{
		value_ = GetValue();
		if (ranges_specified_)
		{
			if (!(value_ >= min_value_ && value_ <= max_value_))
			{
				throw MemoriaException(MEMORIA_SOURCE, "Range checking failure for the property: "+set_name_+"."+name_);
			}
		}
	}

	virtual String GetPropertyName()
	{
		return set_name_+"."+name_;
	}

	virtual void Dump(std::ostream& os)
	{
		if (default_value_specified_ || ranges_specified_) {
			os<<"#";
		}

		if (default_value_specified_)
		{
			os<<"default: "<<default_value_<<" ";
		}

		if (ranges_specified_)
		{
			os<<"Range from: "<<min_value_<<" to "<<max_value_;
		}
	}

protected:
	T GetValue();
};


class TaskParametersSet {
	String 			name_;
	Configurator* 	cfg_;
	bool			enabled_;

	vector<AbstractParamDescriptor*> descriptors_;

public:
	TaskParametersSet(String name): name_(name)
	{
		Add("enabled", enabled_, true);
	}

	bool IsEnabled() const
	{
		return enabled_;
	}

	void SetCfg(Configurator* cfg)
	{
		cfg_ = cfg;
		this->Process();
	}

	const Configurator* GetCfg() const
	{
		return cfg_;
	}

	Configurator* GetCfg()
	{
		return cfg_;
	}

	StringRef GetName() const
	{
		return name_;
	}

	template <typename T>
	void Add(StringRef name, T& property)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, name_, name, property));
	}

	template <typename T>
	void Add(StringRef name, T& property, const T& default_value)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, name_, name, property, default_value));
	}

	template <typename T>
	void Add(StringRef name, T& property, const T& default_value, const T& max_value)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, name_, name, property, default_value, max_value));
	}

	template <typename T>
	void Add(StringRef name, T& property, const T& default_value, const T& min_value, const T& max_value)
	{
		descriptors_.push_back(new ParamDescriptor<T>(this, name_, name, property, default_value, min_value, max_value));
	}

	void DumpProperties(std::ostream& os);
	void Process();
};



template <typename T>
T ParamDescriptor<T>::GetValue()
{
	Configurator* cfg = cfg_->GetCfg();

	String ext_name = set_name_ + "." + name_;
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
