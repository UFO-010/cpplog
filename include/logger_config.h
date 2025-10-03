#ifndef LOGGERCONFIG_H
#define LOGGERCONFIG_H

#include <cstddef>

/// Maximum number of sinks to place logger data
constexpr int LOGGER_MAX_SINKS = 4;
/// Maximum length of output string
constexpr size_t LOGGER_MAX_STR_SIZE = 512;
/// Maximum length of string to place time and data
constexpr size_t LOGGER_MAX_TEMP_SIZE = 32;
/// Maximum length of string to place numbers
constexpr size_t LOGGER_MAX_NUMBUF_SIZE = 12;
/// Maximum number of tokens to search in log message pattern
constexpr size_t LOGGER_MAX_TOKENS = 9;
/// Maximum length of token to search in log message pattern
constexpr size_t LOGGER_LITERAL_BUFFER_SIZE = 64;

#ifndef ENABLE_PRINT_CALLBACK
    #define ENABLE_PRINT_CALLBACK 0  // user callbacks disabled by default
#endif

#ifndef ENABLE_SINKS
    #define ENABLE_SINKS 1  // user sinks enabled by default
#endif

#ifndef LOGGER_MAX_LEVEL
    #define LOGGER_MAX_LEVEL 4  // Debug by default
#endif

/// Macros to check logging level during preprocess. Silences logging levels even if setup level in
/// code is another. Printing log via direct call of log function still can be performed.
#define LOGGER_LOG_DEBUG_ENABLED (LOGGER_MAX_LEVEL >= 4)
#define LOGGER_LOG_INFO_ENABLED (LOGGER_MAX_LEVEL >= 3)
#define LOGGER_LOG_WARNING_ENABLED (LOGGER_MAX_LEVEL >= 2)
#define LOGGER_LOG_ERROR_ENABLED (LOGGER_MAX_LEVEL >= 1)
#define LOGGER_LOG_FATAL_ENABLED (LOGGER_MAX_LEVEL >= 0)

#endif
