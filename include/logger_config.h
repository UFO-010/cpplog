#ifndef LOGGERCONFIG_H
#define LOGGERCONFIG_H

#include <cstddef>

namespace Log {

enum class level : int {
    FatalMsg = 0,
    ErrorMsg = 1,
    WarningMsg = 2,
    InfoMsg = 3,
    DebugMsg = 4,
};

}  // namespace Log

namespace Log::Config {
struct BaseTraits {
    /// Maximum number of sinks to place logger data
    static constexpr int LOGGER_MAX_SINKS = 4;
    /// Maximum length of output string
    static constexpr size_t LOGGER_MAX_STR_SIZE = 512;
    /// Maximum length of logger input message
    static constexpr size_t LOGGER_MAX_MESSAGE_SIZE = 256;
    static constexpr size_t LOGGER_MAX_FORMAT_SIZE = 128;
    /// Maximum length of string to place numbers
    static constexpr size_t LOGGER_MAX_NUMBUF_SIZE = 12;
    /// Maximum number of tokens to search in log message pattern
    static constexpr size_t LOGGER_MAX_TOKENS = 9;
    /// Maximum length of token to search in log message pattern
    static constexpr size_t LOGGER_LITERAL_BUFFER_SIZE = 64;

    /// Enables callbacks to print log messages during compile time
    static constexpr bool ENABLE_PRINT_CALLBACK = false;  // callback disabled by default
    /// Enables sinks to print log messages during compile time
    static constexpr bool ENABLE_SINKS = true;  // sinks enabled by default

    static constexpr int LOGGER_MAX_LEVEL = 4;  // Debug by default

    /// Check logging level during preprocess. Silences logging levels even if setup level in
    /// code is another. Printing log via direct call of log function still can be performed.
    static constexpr bool FATAL_ENABLED = LOGGER_MAX_LEVEL >= static_cast<int>(level::FatalMsg);
    static constexpr bool ERROR_ENABLED = LOGGER_MAX_LEVEL >= static_cast<int>(level::ErrorMsg);
    static constexpr bool WARNING_ENABLED = LOGGER_MAX_LEVEL >= static_cast<int>(level::WarningMsg);
    static constexpr bool INFO_ENABLED = LOGGER_MAX_LEVEL >= static_cast<int>(level::InfoMsg);
    static constexpr bool DEBUG_ENABLED = LOGGER_MAX_LEVEL >= static_cast<int>(level::DebugMsg);
};

template <typename ConfigTag>
struct Traits : BaseTraits {};

struct Default {};

template <>
struct Traits<Default> : public BaseTraits {};

}  // namespace Log::Config

#endif
