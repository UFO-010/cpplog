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

#define ENABLE_PRINT_CALLBACK 0
#define ENABLE_SINKS 1

#endif
