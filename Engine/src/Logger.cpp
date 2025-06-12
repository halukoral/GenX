#include "Logger.h"

// Define the global logger instance that was only declared in the header
Logger globalLogger;

// Initialize the logger if needed
//static void InitializeLogger()
//{
//	Configure your logger here
//	For example: globalLogger.SetLogLevel(LogLevel::Debug);
//}

const std::string Logger::Colors::RESET = "\033[0m";
const std::string Logger::Colors::RED = "\033[31m";
const std::string Logger::Colors::GREEN = "\033[32m";
const std::string Logger::Colors::YELLOW = "\033[33m";
const std::string Logger::Colors::BLUE = "\033[34m";
const std::string Logger::Colors::MAGENTA = "\033[35m";
const std::string Logger::Colors::CYAN = "\033[36m";
const std::string Logger::Colors::WHITE = "\033[37m";
const std::string Logger::Colors::BRIGHT_RED = "\033[91m";
const std::string Logger::Colors::BRIGHT_GREEN = "\033[92m";
const std::string Logger::Colors::BRIGHT_YELLOW = "\033[93m";
const std::string Logger::Colors::BRIGHT_BLUE = "\033[94m";
const std::string Logger::Colors::BRIGHT_MAGENTA = "\033[95m";
const std::string Logger::Colors::BRIGHT_CYAN = "\033[96m";
const std::string Logger::Colors::BRIGHT_WHITE = "\033[97m";