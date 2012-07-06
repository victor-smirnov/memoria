
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_API_MODELS_LOGS_HPP
#define	_MEMORIA_CORE_API_MODELS_LOGS_HPP

#include <memoria/core/tools/id.hpp>
#include <iostream>
#include <iomanip>

#define MEMORIA_LOG(logger_, level, ...)                                         \
    if (logger_->is_log(level))                                                    \
        memoria::vapi::log(logger_->logger(), level, MEMORIA_SOURCE, logger_->typeName(), ExtractFunctionName(__FUNCTION__), ##__VA_ARGS__)

#define MEMORIA_DEBUG(logger_, ...)                                              \
    MEMORIA_LOG(logger_, memoria::vapi::Logger::DEBUG, ##__VA_ARGS__)

#define MEMORIA_WARN(logger_, ...)                                               \
    MEMORIA_LOG(logger_, memoria::vapi::Logger::WARN, ##__VA_ARGS__)

#define MEMORIA_ERROR(logger_, ...)                                              \
    MEMORIA_LOG(logger_, memoria::vapi::Logger::ERROR, ##__VA_ARGS__)

#define MEMORIA_INFO(logger_, ...)                                               \
    MEMORIA_LOG(logger_, memoria::vapi::Logger::INFO, ##__VA_ARGS__)

#define MEMORIA_TRACE(logger_, ...)                                              \
    MEMORIA_LOG(logger_, memoria::vapi::Logger::TRACE, ##__VA_ARGS__)


namespace memoria    {
namespace vapi       {


const char* ExtractFunctionName(const char* full_name);


struct MEMORIA_API LogHandler {

    LogHandler() {}

    virtual void begin(Int level)       = 0;

    virtual void log(const bool value)        = 0;
    virtual void log(const Byte value)        = 0;
    virtual void log(const UByte value)       = 0;
    virtual void log(const Short value)       = 0;
    virtual void log(const UShort value)      = 0;
    virtual void log(const Int value)         = 0;
    virtual void log(const UInt value)        = 0;
    virtual void log(const BigInt value)      = 0;
    virtual void log(const UBigInt value) 	  = 0;
    virtual void log(const float value)       = 0;
    virtual void log(const double value)      = 0;
    virtual void log(const IDValue& value)    = 0;
    virtual void log(StringRef value)   = 0;
    virtual void log(const char* value) = 0;
    virtual void log(const void* value) = 0;

    virtual void log(...) = 0;

    virtual void end()                  = 0;


};

class Logger;
extern Logger logger;

class Logger {

	const char* category_;
	int level_;

	Logger* parent_;
	LogHandler* handler_;

	static LogHandler* default_handler_;

	friend void initContainers();

public:

	enum {DERIVED = 0, TRACE = 10000, DEBUG = 20000, ERROR = 30000, WARNING = 40000, INFO = 50000, FATAL = 60000, NONE = 70000};

	Logger(const char* category, int level = DERIVED, Logger* parent = &memoria::vapi::logger):category_(category), level_(level), parent_(parent), handler_(NULL)
	{}

	Logger(const Logger& other):category_(other.category_), level_(other.level_), parent_(other.parent_), handler_(other.handler_)
	{}

	const char* category() const {
		return category_;
	}

	int level() const {
		return level_;
	}

	int& level() {
		return level_;
	}

	bool IsLogEnabled(int level)
	{
		int tmp;

		if (level_ != DERIVED)
		{
			tmp = level_;
		}
		else if (parent_ != NULL)
		{
			return parent_->IsLogEnabled(level);
		}
		else {
			tmp = INFO;
		}

		bool result = (tmp != NONE) ? level >= tmp : false;
		return result;
	}

	LogHandler* getHandler() const
	{
		if (handler_ != NULL) {
			return handler_;
		}
		else if (parent_ != NULL) {
			return parent_->getHandler();
		}
		else {
			return default_handler_;
		}
	}

	void setHandler(LogHandler* handler)
	{
		handler_ = handler;
	}

	Logger& logger() {
		return *this;
	}
};




class DefaultLogHandlerImpl: public LogHandler {

	Int cnt_;

	std::ostream& out_;

public:

	DefaultLogHandlerImpl(): out_(std::cout) {}
	DefaultLogHandlerImpl(std::ostream& out): out_(out) {}

    virtual void begin(Int level) {
    	cnt_ = 0;
    	preprocess();
        if (level <= Logger::TRACE)         out_<<"TRACE";
        else if (level <= Logger::DEBUG)    out_<<"DEBUG";
        else if (level <= Logger::INFO)     out_<<"INFO";
        else if (level <= Logger::WARNING)  out_<<"WARNING";
        else if (level <= Logger::ERROR)    out_<<"ERROR";
        else out_<<"FATAL";
        out_<<" ";
        postprocess();
    }

    virtual void log(const bool value)      {preprocess(); out_<<value; postprocess();}
    virtual void log(const Byte value)      {preprocess(); out_<<value; postprocess();}
    virtual void log(const UByte value)     {preprocess(); out_<<value; postprocess();}
    virtual void log(const Short value)     {preprocess(); out_<<value; postprocess();}
    virtual void log(const UShort value)    {preprocess(); out_<<value; postprocess();}
    virtual void log(const Int value)       {preprocess(); out_<<value; postprocess();}
    virtual void log(const UInt value)      {preprocess(); out_<<value; postprocess();}
    virtual void log(const BigInt value)    {preprocess(); out_<<value; postprocess();}
    virtual void log(const UBigInt value) 	{preprocess(); out_<<value; postprocess();}
    virtual void log(const float value)     {preprocess(); out_<<value; postprocess();}
    virtual void log(const double value)    {preprocess(); out_<<value; postprocess();}
    virtual void log(const IDValue& value)  {preprocess(); out_<<value.str(); postprocess();}
    virtual void log(StringRef value)       {preprocess(); out_<<value; postprocess();}
    virtual void log(const char* value) 	{preprocess(); out_<<value; postprocess();}
    virtual void log(const void* value) 	{preprocess(); out_<<value; postprocess();}

    virtual void log(...) 					{preprocess(); out_<<"DEFAULT"; postprocess();}

    virtual void end() {
        out_<<std::endl;
    }

    void preprocess()
    {
    	using namespace std;
    	if (cnt_ == 0) {
    		out_<<left<<setw(7);
    	}
    	else if (cnt_ == 1) {
    		out_<<left<<setw(75);
    	}
    	else if (cnt_ == 3) {
    		out_<<left<<setw(25);
    	}
    	else if (cnt_ == 5) {
    		out_<<left<<setw(22);
    	}
    }

    void postprocess()
    {
    	cnt_++;
    }

};


class Locker {
	LogHandler* handler_;

public:
	Locker(Logger& logger, Int level): handler_(logger.getHandler())
	{
		handler_->begin(level);
	}

	LogHandler* handler() {
		return handler_;
	}

	~Locker() throw() {
		try {
			handler_->end();
		}
		catch (...) {}
	}
};





template <typename T>
LogHandler* logIt(LogHandler* log, const T& value) {
    log->log(value);
    log->log(" ");
    return log;
}



template <typename T0>
bool log(Logger& log, Int level, const T0& v0) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = log.getHandler();

        logIt(handler, v0);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <typename T0, typename T1>
bool log(Logger& log, Int level, const T0& v0, const T1& v1) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <typename T0, typename T1, typename T2>
bool log(Logger& log, Int level, const T0& v0, const T1& v1, const T2& v2) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <typename T0, typename T1, typename T2, typename T3>
bool log(Logger& log, Int level, const T0& v0, const T1& v1, const T2& v2, const T3& v3) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <typename T0, typename T1, typename T2, typename T3, typename T4>
bool log(Logger& log, Int level, const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6
>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5, const T6& v6) throw () {
    try {
    	Locker lock(log, level);
    	LogHandler* handler = lock.handler();

    	logIt(handler, v0);
    	logIt(handler, v1);
    	logIt(handler, v2);
    	logIt(handler, v3);
    	logIt(handler, v4);
    	logIt(handler, v5);
    	logIt(handler, v6);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7
>
bool log(Logger& log, Int level,
         const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
         const T5& v5, const T6& v6, const T7& v7) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8
>
bool log(Logger& log, Int level,
         const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
         const T5& v5, const T6& v6, const T7& v7, const T8& v8) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);
        logIt(handler, v8);


        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8, typename T9
>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);
        logIt(handler, v8);
        logIt(handler, v9);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8, typename T9,
        typename T10
>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9,
        const T10& v10) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);
        logIt(handler, v8);
        logIt(handler, v9);
        logIt(handler, v10);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8, typename T9,
        typename T10, typename T11
>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9,
        const T10& v10, const T11& v11) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);
        logIt(handler, v8);
        logIt(handler, v9);
        logIt(handler, v10);
        logIt(handler, v11);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8, typename T9,
        typename T10, typename T11, typename T12
>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9,
        const T10& v10, const T11& v11, const T12& v12) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);
        logIt(handler, v8);
        logIt(handler, v9);
        logIt(handler, v10);
        logIt(handler, v11);
        logIt(handler, v12);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8, typename T9,
        typename T10, typename T11, typename T12, typename T13
>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9,
        const T10& v10, const T11& v11, const T12& v12, const T13& v13) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);
        logIt(handler, v8);
        logIt(handler, v9);
        logIt(handler, v10);
        logIt(handler, v11);
        logIt(handler, v12);
        logIt(handler, v13);

        return true;
    }
    catch (...) {
        return false;
    }
}

template <
        typename T0, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8, typename T9,
        typename T10, typename T11, typename T12, typename T13, typename T14
>
bool log(Logger& log, Int level,
        const T0& v0, const T1& v1, const T2& v2, const T3& v3, const T4& v4,
        const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9,
        const T10& v10, const T11& v11, const T12& v12, const T13& v13, const T14& v14) throw () {
    try {
        Locker lock(log, level);
        LogHandler* handler = lock.handler();

        logIt(handler, v0);
        logIt(handler, v1);
        logIt(handler, v2);
        logIt(handler, v3);
        logIt(handler, v4);
        logIt(handler, v5);
        logIt(handler, v6);
        logIt(handler, v7);
        logIt(handler, v8);
        logIt(handler, v9);
        logIt(handler, v10);
        logIt(handler, v11);
        logIt(handler, v12);
        logIt(handler, v13);
        logIt(handler, v14);

        return true;
    }
    catch (...) {
        return false;
    }
}

/*
static String expand(StringRef str) {
	unsigned int LEN = 62;
	if (str.length() < LEN)
	{
		String st(str);
		for (size_t c = 0; c < LEN - str.length(); c++)
		{
			st.append(" ");
		}
		return st;
	}
	else {
		return str;
	}
}
*/



}}



#endif
