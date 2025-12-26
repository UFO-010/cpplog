#ifndef LOGGER_H
#define LOGGER_H

#include <cstring>
#include <charconv>
#include <tuple>
#include <array>
#include <string_view>
#include <functional>

/// @todo remplace with shrunked fmtlib
#include <format>

#include "logger_config.h"
#include "message.h"

#if defined(__GNUC__) || defined(__clang__)
    #define LOG_CURRENT_FUNC __PRETTY_FUNCTION__
#elif _MSC_VER && !__INTEL_COMPILER
    #define LOG_CURRENT_FUNC __FUNCSIG__
#else
    #define LOG_CURRENT_FUNC __func__
#endif

#define Debug(LoggerType, fmt, ...) \
    LoggerType.debug(fmt, __FILE__, LOG_CURRENT_FUNC, __LINE__, ##__VA_ARGS__)
#define Info(LoggerType, fmt, ...) \
    LoggerType.info(fmt, __FILE__, LOG_CURRENT_FUNC, __LINE__, ##__VA_ARGS__)
#define Warning(LoggerType, fmt, ...) \
    LoggerType.warning(fmt, __FILE__, LOG_CURRENT_FUNC, __LINE__, ##__VA_ARGS__)
#define Error(LoggerType, fmt, ...) \
    LoggerType.error(fmt, __FILE__, LOG_CURRENT_FUNC, __LINE__, ##__VA_ARGS__)
#define Fatal(LoggerType, fmt, ...) \
    LoggerType.fatal(fmt, __FILE__, LOG_CURRENT_FUNC, __LINE__, ##__VA_ARGS__)

namespace Log {
template <typename Derived>
class ILogSink {
public:
    void send(const level msgType, const char *data, size_t size) const {
        static_cast<const Derived *>(this)->sendImpl(msgType, data, size);
    }
};

/**
 * @brief The Logger class
 *
 * Main logging class. Uses DataProvider implemented by user to get platform-specific data.
 *
 * @todo Add queue interface to implement async work
 */
template <typename TContextProvider,
          typename ConfigTag = Config::Traits<Config::Default>,
          typename... TSinkTypes>
class Logger {
public:
    using TConfig = Log::Config::Traits<ConfigTag>;
    using CallbackType = std::function<void(const level, const char *, size_t)>;
    using TMessage = LogMessage<TConfig>;

    explicit Logger(const TContextProvider &provider, TSinkTypes... sink_args) noexcept
        : data_provider_instance(provider),
          sinks_tuple(sink_args...) {
        setLogPattern("%{level}: %{message}");  // default pattern
    }

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    /**
     * @brief setLogLevel
     * @param level priority of messages to display
     *
     * Set logging level. Messages with a lower priority level will be ignored
     */
    void setLogLevel(const level lev) { logLevel = static_cast<int>(lev); }

    /**
     * @brief getLevel
     * @return minimum logging level
     */
    int getLevel() const { return logLevel; }

    /**
     * @brief setLogPattern
     * @param pattern Output message pattern
     *
     * Define ouput logging message format to look like.
     * Options : "%{date}"; "%{time}"; "%{level}"; "%{file}"; "%{thread}";
     * "%{function}"; "%{line}"; "%{pid}"; "%{message}".
     * @example "%{date} %{time}"
     * Output: "<current date> <current time>"
     * All text after the last token would be ignored.
     */
    bool setLogPattern(const char *pattern) {
        tokenOpsCount = 0;
        size_t literal_buffer_pos = 0;

        const char *p = pattern;
        const char *start_of_literal = p;
        char *literal = literalBuffer.data();

        while (*p != '\0' && tokenOpsCount < TConfig::LOGGER_MAX_TOKENS) {
            if (*p != '%') {
                ++p;
                continue;
            }
            if (*(p + 1) != '{') {
                ++p;
                continue;
            }

            const char *token_start = p;
            const char *brace_end = strchr(p + 2, '}');
            if (brace_end == nullptr) {
                break;
            }

            auto literal_len = static_cast<size_t>(token_start - start_of_literal);

            if (literal_buffer_pos + literal_len > TConfig::LOGGER_LITERAL_BUFFER_SIZE) {
                literal_len = TConfig::LOGGER_LITERAL_BUFFER_SIZE - literal_buffer_pos;
            }

            if (literal_buffer_pos >= TConfig::LOGGER_LITERAL_BUFFER_SIZE) {
                break;
            }

            char *dest = literal + literal_buffer_pos;
            if (literal_len > 0) {
                std::memcpy(dest, start_of_literal, literal_len);
            }
            literal_buffer_pos += literal_len;

            auto token_len = static_cast<size_t>(brace_end - token_start + 1);
            tokType found_type = tokType::TokInvalid;
            std::string_view token_sv(token_start, token_len);
            for (size_t i = 0; i < tokens.size(); ++i) {
                if (tokens[i].size() == token_len && token_sv == tokens[i]) {
                    found_type = static_cast<tokType>(i);
                    break;
                }
            }

            tokenOps[tokenOpsCount] = {found_type, dest, literal_len};
            ++tokenOpsCount;
            p = brace_end + 1;
            start_of_literal = p;
        }
        return true;
    }

    void setUserHandler(const CallbackType &_handler) { userHandler = _handler; }

    /**
     * @brief fatal
     * @todo replace std after moving to fmt
     */
    template <typename... Args>
    void fatal(const std::format_string<Args...> &fmt,
               const std::string_view &file,
               const std::string_view &func,
               const size_t line,
               Args &&...args) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::FatalMsg)) {
                LogMessage<TConfig> msg{.record{level::FatalMsg, file, func, line},
                                        .user_data = {},
                                        .user_data_len = 0,
                                        .timestamp = data_provider_instance.getTimestamp()};

                auto res = std::format_to_n(msg.user_data.data(), msg.user_data.size(), fmt,
                                            std::forward<Args>(args)...);
                msg.user_data_len = res.size;
                log(msg);
            }
        }
    }

    template <typename... Args>
    void error(const std::format_string<Args...> &fmt,
               const std::string_view &file,
               const std::string_view &func,
               const size_t line,
               Args &&...args) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::ErrorMsg)) {
                LogMessage<TConfig> msg{.record{level::ErrorMsg, file, func, line},
                                        .user_data = {},
                                        .user_data_len = 0,
                                        .timestamp = data_provider_instance.getTimestamp()};

                auto res = std::format_to_n(msg.user_data.data(), msg.user_data.size(), fmt,
                                            std::forward<Args>(args)...);
                msg.user_data_len = res.size;
                log(msg);
            }
        }
    }

    template <typename... Args>
    void warning(const std::format_string<Args...> &fmt,
                 const std::string_view &file,
                 const std::string_view &func,
                 const size_t line,
                 Args &&...args) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::WarningMsg)) {
                LogMessage<TConfig> msg{.record{level::WarningMsg, file, func, line},
                                        .user_data = {},
                                        .user_data_len = 0,
                                        .timestamp = data_provider_instance.getTimestamp()};

                auto res = std::format_to_n(msg.user_data.data(), msg.user_data.size(), fmt,
                                            std::forward<Args>(args)...);
                msg.user_data_len = res.size;
                log(msg);
            }
        }
    }

    template <typename... Args>
    void info(const std::format_string<Args...> &fmt,
              const std::string_view &file,
              const std::string_view &func,
              const size_t line,
              Args &&...args) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::InfoMsg)) {
                LogMessage<TConfig> msg{.record{level::InfoMsg, file, func, line},
                                        .user_data = {},
                                        .user_data_len = 0,
                                        .timestamp = data_provider_instance.getTimestamp()};

                auto res = std::format_to_n(msg.user_data.data(), msg.user_data.size(), fmt,
                                            std::forward<Args>(args)...);
                msg.user_data_len = res.size;
                log(msg);
            }
        }
    }

    template <typename... Args>
    void debug(const std::format_string<Args...> &fmt,
               const std::string_view &file,
               const std::string_view &func,
               const size_t line,
               Args &&...args) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::DebugMsg)) {
                LogMessage<TConfig> msg{.record{level::DebugMsg, file, func, line},
                                        .user_data = {},
                                        .user_data_len = 0,
                                        .timestamp = data_provider_instance.getTimestamp()};

                auto res = std::format_to_n(msg.user_data.data(), msg.user_data.size(), fmt,
                                            std::forward<Args>(args)...);
                msg.user_data_len = res.size;
                log(msg);
            }
        }
    }

    /**
     * @brief log
     * @param msg
     *
     * Main logging function. Calls provided sinks and callbacks if enabled in
     * `logger_config.h`. All logging calls can be disabled in the same file.
     */
    void log(const TMessage &msg) const {
        std::array<char, TConfig::LOGGER_MAX_STR_SIZE> finaL_msg;
        size_t msg_size = createMessage(finaL_msg.data(), msg);

        if constexpr (TConfig::ENABLE_SINKS) {
            send_to_all_sinks(msg.record.msgType, finaL_msg.data(), msg_size);
        }

        if constexpr (TConfig::ENABLE_PRINT_CALLBACK) {
            if (userHandler != nullptr) {
                userHandler(msg.record.msgType, finaL_msg.data(), msg_size);
            }
        }
    }

    size_t createMessage(char *outBuf, const TMessage &msg) const {
        size_t pos = 0;
        size_t bufSize = TConfig::LOGGER_MAX_STR_SIZE;

        for (size_t i = 0; i < tokenOpsCount; i++) {
            append(pos, outBuf, bufSize, tokenOps[i].literal, tokenOps[i].literal_len);
            switch (tokenOps[i].type) {
                case tokType::TokDate:
                    tokDateHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokTime:
                    tokTimeHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokLevel:
                    tokLevelHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokFile:
                    tokFileHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokThread:
                    tokThreadHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokFunc:
                    tokFuncHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokLine:
                    tokLineHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokPid:
                    tokPidHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokMessage:
                    tokMessageHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                case tokType::TokInvalid:
                    tokInvalidHandler(pos, outBuf, bufSize, msg, data_provider_instance);
                    break;
                default:
                    break;
            }
        }

        outBuf[pos] = '\0';

        return pos;
    }

private:
    enum class tokType : int {
        TokDate,
        TokTime,
        TokLevel,
        TokFile,
        TokThread,
        TokFunc,
        TokLine,
        TokPid,
        TokMessage,
        TokInvalid
    };

    /**
     * @brief The TokenOp class
     *
     * Holds all tokens and their data found during parsing message pattern in `setLogPattern`
     */
    struct TokenOp {
        /// token type
        tokType type;
        /// pointer to the text that comes before token
        const char *literal;
        /// length of  text that comes before token
        size_t literal_len;
    };

    /**
     * @brief append
     * @param pos position to place sting in outBuf
     * @param outBuf buffer to place string
     * @param bufSize size of buffer
     * @param data string to place in buffer
     * @param dataLen string length
     *
     * Places string in buffer in needed position and increase `pos` to `dataLen` if data is
     * placed.
     */
    static void append(
        size_t &pos, char *outBuf, size_t bufSize, const char *data, size_t dataLen) {
        if (pos + dataLen < bufSize) {
            std::memcpy(outBuf + pos, data, dataLen);
            pos += dataLen;
        }
    }

    static void tokDateHandler(size_t &pos,
                               char *outBuf,
                               [[maybe_unused]] size_t bufSize,
                               [[maybe_unused]] const TMessage &msg,
                               const TContextProvider &data_provider_instance) {
        pos += data_provider_instance.getCurrentDate(outBuf + pos, bufSize - pos);
    }

    static void tokTimeHandler(size_t &pos,
                               char *outBuf,
                               [[maybe_unused]] size_t bufSize,
                               [[maybe_unused]] const TMessage &msg,
                               const TContextProvider &data_provider_instance) {
        pos += data_provider_instance.getCurrentTime(outBuf + pos, bufSize - pos);
    }

    static void tokThreadHandler(size_t &pos,
                                 char *outBuf,
                                 [[maybe_unused]] size_t bufSize,
                                 [[maybe_unused]] const TMessage &msg,
                                 const TContextProvider &data_provider_instance) {
        pos += data_provider_instance.getThreadId(outBuf + pos, bufSize - pos);
    }

    static void tokPidHandler(size_t &pos,
                              char *outBuf,
                              [[maybe_unused]] size_t bufSize,
                              [[maybe_unused]] const TMessage &msg,
                              const TContextProvider &data_provider_instance) {
        pos += data_provider_instance.getProcessName(outBuf + pos, bufSize - pos);
    }

    static void tokLevelHandler(size_t &pos,
                                char *outBuf,
                                size_t bufSize,
                                const TMessage &msg,
                                [[maybe_unused]] const TContextProvider &data_provider_instance) {
        const char *ch = msg_log_types[static_cast<int>(msg.record.msgType)].data();
        size_t len = msg_log_types[static_cast<int>(msg.record.msgType)].size();

        append(pos, outBuf, bufSize, ch, len);
    }

    static void tokFileHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const TMessage &msg,
                               [[maybe_unused]] const TContextProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, msg.record.file.data(), msg.record.file.size());
    }

    static void tokFuncHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const TMessage &msg,
                               [[maybe_unused]] const TContextProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, msg.record.func.data(), msg.record.func.size());
    }

    static void tokLineHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const TMessage &msg,
                               [[maybe_unused]] const TContextProvider &data_provider_instance) {
        char *tail = outBuf + pos;
        std::to_chars_result result = std::to_chars(tail, tail + (bufSize - pos), msg.record.line);
        pos += result.ptr - tail;
    }

    static void tokMessageHandler(size_t &pos,
                                  char *outBuf,
                                  size_t bufSize,
                                  const TMessage &msg,
                                  [[maybe_unused]] const TContextProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, msg.user_data.data(), msg.user_data_len);
    }

    static void tokInvalidHandler([[maybe_unused]] size_t &pos,
                                  [[maybe_unused]] char *outBuf,
                                  [[maybe_unused]] size_t bufSize,
                                  [[maybe_unused]] const TMessage &msg,
                                  [[maybe_unused]] const TContextProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, "invalid token", sizeof("invalid token"));
    }

    int logLevel = 3;
    /// holds all text before tokens found in `setLogPattern`. Token itself holds legth and
    /// pointer to text that comes before him, @see TokenOp
    std::array<char, TConfig::LOGGER_LITERAL_BUFFER_SIZE> literalBuffer = {};
    /// found tokens, so the output will look the same as `setLogPattern`
    std::array<TokenOp, TConfig::LOGGER_MAX_TOKENS> tokenOps = {};
    /// number of found tokens
    size_t tokenOpsCount = 0;

    /// class that provides platform-dependent data
    TContextProvider data_provider_instance;

    // TMessageQueue &queue;
    /// holds sinks to send log messages to
    std::tuple<TSinkTypes...> sinks_tuple;

    /**
     * @brief send_to_all_sinks
     * @param msgType log level
     * @param data log message
     * @param size size of log message
     * @return
     *
     * Recursively send log message to all user sinks
     */
    template <std::size_t I = 0>
    inline void send_to_all_sinks(const level &msgType, const char *data, size_t size) const {
        if constexpr (I < sizeof...(TSinkTypes)) {
            // call current sink
            std::get<I>(sinks_tuple).send(msgType, data, size);
            // call next sink
            send_to_all_sinks<I + 1>(msgType, data, size);
        }
    }

    /// user callback to print logging message
    CallbackType userHandler;

    /// types of logging level, added to output message
    static constexpr std::array<std::string_view, 5> msg_log_types = {"FATAL", "ERROR", "WARN",
                                                                      "INFO", "DEBUG"};
    /// tokens for message pattern
    static constexpr std::array<std::string_view, 9> tokens = {
        "%{date}",     "%{time}", "%{level}", "%{file}",   "%{thread}",
        "%{function}", "%{line}", "%{pid}",   "%{message}"};
};

}  // namespace Log

#endif
