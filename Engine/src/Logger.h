#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

class Logger
{
public:
    enum LogLevel
	{
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

private:
	
    LogLevel currentLevel;
    std::ofstream logFile;
    bool logToConsole;
    bool logToFile;
    std::mutex logMutex;

    std::string getCurrentTimestamp()
	{
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    std::string levelToString(LogLevel level)
	{
        switch (level)
    	{
            case DEBUG: return "DEBUG:";
            case INFO:  return "INFO:";
            case WARN:  return "WARN:";
            case ERROR: return "ERROR:";
            default:    return "UNKNOWN:";
        }
    }

    void writeLog(LogLevel level, const std::string& message)
	{
        if (level < currentLevel) return;

        std::lock_guard<std::mutex> lock(logMutex);
        
        std::string timestamp = getCurrentTimestamp();
        std::string levelStr = levelToString(level);
        std::string logEntry = "[" + timestamp + "] [" + levelStr + "] " + message;

        if (logToConsole)
        {
            std::cout << logEntry << '\n';
        }

        if (logToFile && logFile.is_open())
        {
            logFile << logEntry << '\n';
            logFile.flush();
        }
    }

public:
    Logger(const LogLevel level = INFO, const bool console = true, const bool file = false, 
           const std::string& filename = "app.log") 
        : currentLevel(level), logToConsole(console), logToFile(file)
	{
        
        if (logToFile)
        {
            logFile.open(filename, std::ios::app);
            if (!logFile.is_open())
            {
                std::cerr << "Failed to open log file: " << filename << '\n';
                logToFile = false;
            }
        }
    }

    ~Logger() {
        if (logFile.is_open())
        {
            logFile.close();
        }
    }

    void setLogLevel(const LogLevel level)
	{
        currentLevel = level;
    }

    void setConsoleOutput(const bool enabled)
	{
        logToConsole = enabled;
    }

    void setFileOutput(const bool enabled, const std::string& filename = "")
	{
        std::lock_guard<std::mutex> lock(logMutex);
        
        if (logFile.is_open())
        {
            logFile.close();
        }
        
        logToFile = enabled;
        if (enabled)
        {
            std::string file = filename.empty() ? "app.log" : filename;
            logFile.open(file, std::ios::app);
            if (!logFile.is_open())
            {
                std::cerr << "Failed to open log file: " << file << '\n';
                logToFile = false;
            }
        }
    }

    void debug(const std::string& message)
	{
        writeLog(DEBUG, message);
    }

    void info(const std::string& message)
	{
        writeLog(INFO, message);
    }

    void warn(const std::string& message)
	{
        writeLog(WARN, message);
    }

    void error(const std::string& message)
	{
        writeLog(ERROR, message);
    }

    // Template version for formatted logging
    template<typename... Args>
    void debug(const std::string& format, Args... args)
	{
        writeLog(DEBUG, formatString(format, args...));
    }

    template<typename... Args>
    void info(const std::string& format, Args... args)
	{
        writeLog(INFO, formatString(format, args...));
    }

    template<typename... Args>
    void warn(const std::string& format, Args... args)
	{
        writeLog(WARN, formatString(format, args...));
    }

    template<typename... Args>
    void error(const std::string& format, Args... args)
	{
        writeLog(ERROR, formatString(format, args...));
    }

private:
    // Simple string formatting helper
    template<typename... Args>
    std::string formatString(const std::string& format, Args... args)
	{
        std::stringstream ss;
        formatHelper(ss, format, args...);
        return ss.str();
    }

    template<typename T>
    void formatHelper(std::stringstream& ss, const std::string& format, T&& t)
	{
        size_t pos = format.find("{}");
        if (pos != std::string::npos)
        {
            ss << format.substr(0, pos) << t << format.substr(pos + 2);
        }
    	else
    	{
            ss << format;
        }
    }

    template<typename T, typename... Args>
    void formatHelper(std::stringstream& ss, const std::string& format, T&& t, Args&&... args)
	{
        size_t pos = format.find("{}");
        if (pos != std::string::npos)
        {
            ss << format.substr(0, pos) << t;
            formatHelper(ss, format.substr(pos + 2), args...);
        }
    	else
    	{
            ss << format;
        }
    }
};

// Global logger instance (optional convenience)
extern Logger globalLogger;

// Convenience macros (optional)
#define LOG_DEBUG(msg) globalLogger.debug(msg)
#define LOG_INFO(msg) globalLogger.info(msg)
#define LOG_WARN(msg) globalLogger.warn(msg)
#define LOG_ERROR(msg) globalLogger.error(msg)

// Ex:
// LOG_INFO("This is a info message");

// Formatted logging
//int userId = 123;
//std::string username = "john_doe";
//LOG_INFO("User {} logged in with ID: {}", username, userId);
    
// Change log level at runtime
//globalLogger.setLogLevel(Logger::WARN);
//LOG_DEBUG("This won't be logged");
//LOG_WARN("This will be logged");