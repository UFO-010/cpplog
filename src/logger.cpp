
#include "logger.h"

namespace Log {
int Logger::log_level = 4;

ILogSink *Logger::sinks[LOGGER_MAX_SINKS] = {};
int Logger::sink_count = 0;
DataProvider *Logger::data_provider = 0;

void (*Logger::user_handler)(const messageType &msgType, const char *message, size_t msg_size) = 0;

Logger::TokenOp Logger::tokenOps[LOGGER_MAX_TOKENS] = {};
size_t Logger::token_ops_count = 0;
char Logger::literal_buffer[LOGGER_LITERAL_BUFFER_SIZE] = {};

}  // namespace Log
