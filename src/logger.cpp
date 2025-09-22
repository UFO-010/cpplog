
#include "logger.h"

namespace Log {
int Logger::log_level = 4;

std::vector<ILogSink *> Logger::sinks = {};
const DataProvider *Logger::data_provider = 0;

const std::string Logger::tok_date = "%{date}";
const std::string Logger::tok_time = "%{time}";
const std::string Logger::tok_type = "%{type}";
const std::string Logger::tok_file = "%{file}";
const std::string Logger::tok_thread = "%{thread}";
const std::string Logger::tok_func = "%{function}";
const std::string Logger::tok_line = "%{line}";
const std::string Logger::tok_pid = "%{pid}";
const std::string Logger::tok_message = "%{message}";

const char *Logger::msg_log_types[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

std::vector<const std::string *> Logger::tokens_pos = {&tok_type, &tok_message};
std::vector<std::string> Logger::tokens_messages = {"\t", ""};

void (*Logger::user_handler)(const messageType &msgType, const std::string &message) = 0;

}  // namespace Log
