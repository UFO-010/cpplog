
#pragma once

#include <array>
#include <string_view>

#include "logger_config.h"

namespace Log {

/**
 * @brief The LogRecord class
 *
 * Holds log data know in compile time
 */
struct LogRecord {
public:
    const level msgType;
    const std::string_view file;
    const std::string_view func;
    const size_t line;

    constexpr LogRecord(const level v_msgType,
                        const std::string_view &v_file,
                        const std::string_view &v_func,
                        size_t v_line) noexcept
        : msgType(v_msgType),
          file(v_file),
          func(v_func),
          line(v_line) {}
};

/**
 * @brief The LogMessage class
 *
 * Holds log context captured in hot path. Should be used with message interface to process data in
 * background
 */
template <typename ConfigTag = Config::Traits<Config::Default>>
struct LogMessage {
    using TConfig = Log::Config::Traits<ConfigTag>;

    LogRecord record;

    std::array<char, TConfig::LOGGER_MAX_FORMAT_SIZE> user_data;
    size_t user_data_len;

    long long timestamp;
};

}  // namespace Log
