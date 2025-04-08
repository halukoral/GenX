#pragma once

#include <cstdio>

class Logger {
public:
	/* log if input log level is equal or smaller to log level set */
	template <typename... Args>
	static void log(unsigned int logLevel, Args ... args) {
		if (logLevel <= mLogLevel) {
			std::printf(args ...);
			/* force output, i.e. for Eclipse */
			std::fflush(stdout);
		}
	}

	static void setLogLevel(unsigned int inLogLevel) {
		inLogLevel <= 9 ? mLogLevel = inLogLevel : mLogLevel = 9;
	}

private:
	static unsigned int mLogLevel;
};

#define LOG_TRACE(...)		Logger::log(1, __VA_ARGS__);		
#define LOG_INFO(...)		Logger::log(2, __VA_ARGS__);
#define LOG_WARN(...)		Logger::log(3, __VA_ARGS__);
#define LOG_ERROR(...)		Logger::log(4, __VA_ARGS__);
#define LOG_CRITICAL(...)	Logger::log(5, __VA_ARGS__);
