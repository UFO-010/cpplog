#ifndef LOGGERCONFIG_H
#define LOGGERCONFIG_H

#include <cstddef>

constexpr int LOGGER_MAX_SINKS = 4;            /// Maximum number of sinks to place logger data
constexpr size_t LOGGER_MAX_STR_SIZE = 512;    /// Maximum length of output string
constexpr size_t LOGGER_MAX_TEMP_SIZE = 32;    /// Maximum length of string to place time and data
constexpr size_t LOGGER_MAX_TOK_SIZE = 64;     /// Maximum length of token
constexpr size_t LOGGER_MAX_NUMBUF_SIZE = 12;  /// Maximum length of string to place numbers
constexpr size_t LOGGER_MAX_MESSAGES = 9;      /// Maximum number of user messages to place between
                                               /// tokens
constexpr size_t LOGGER_MAX_TOKENS = 18;
constexpr size_t LOGGER_LITERAL_BUFFER_SIZE = 64;

#ifndef ENABLE_PRINT_CALLBACK
    #define ENABLE_PRINT_CALLBACK 0  // disabled by default
#endif

#ifndef ENABLE_SINKS
    #define ENABLE_SINKS 1  // enabled by default
#endif

#ifndef LOGGER_MAX_LEVEL
    #define LOGGER_MAX_LEVEL 4  // Debug by default
#endif

#define LOGGER_LOG_DEBUG_ENABLED (LOGGER_MAX_LEVEL >= 4)
#define LOGGER_LOG_INFO_ENABLED (LOGGER_MAX_LEVEL >= 3)
#define LOGGER_LOG_WARNING_ENABLED (LOGGER_MAX_LEVEL >= 2)
#define LOGGER_LOG_ERROR_ENABLED (LOGGER_MAX_LEVEL >= 1)
#define LOGGER_LOG_FATAL_ENABLED (LOGGER_MAX_LEVEL >= 0)

#endif
