
#include "include/logger.h"

namespace Log {
int Logger::logLevel = 4;

ILogSink *Logger::sinks[LOGGER_MAX_SINKS] = {};
int Logger::sinkCount = 0;
DataProvider *Logger::data_provider = 0;

void (*Logger::userHandler)(const messageType &msgType, const char *message, size_t msg_size) = 0;

Logger::TokenOp Logger::tokenOps[LOGGER_MAX_TOKENS] = {};
size_t Logger::tokenOpsCount = 0;
char Logger::literalBuffer[LOGGER_LITERAL_BUFFER_SIZE] = {};

}  // namespace Log
