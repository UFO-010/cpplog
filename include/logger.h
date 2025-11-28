#ifndef LOGGER_H
#define LOGGER_H

#include <cstring>
#include <charconv>
#include <tuple>
#include <array>
#include <string_view>
#include <functional>

#include "logger_config.h"
#include "default_provider.h"

#if defined(__GNUC__) || defined(__clang__)
    #define LOG_CURRENT_FUNC __PRETTY_FUNCTION__
#elif _MSC_VER && !__INTEL_COMPILER
    #define LOG_CURRENT_FUNC __FUNCSIG__
#else
    #define LOG_CURRENT_FUNC __func__
#endif

#define Debug(LoggerType, message, len) \
    LoggerType.debug(message, len, __FILE__, LOG_CURRENT_FUNC, __LINE__);
#define Info(LoggerType, message, len) \
    LoggerType.info(message, len, __FILE__, LOG_CURRENT_FUNC, __LINE__);
#define Warning(LoggerType, message, len) \
    LoggerType.warning(message, len, __FILE__, LOG_CURRENT_FUNC, __LINE__);
#define Error(LoggerType, message, len) \
    LoggerType.error(message, len, __FILE__, LOG_CURRENT_FUNC, __LINE__);
#define Fatal(LoggerType, message, len) \
    LoggerType.fatal(message, len, __FILE__, LOG_CURRENT_FUNC, __LINE__);

namespace Log {

template <typename ConcreteSink>
class ILogSink {
public:
    void send(const level msgType, const char *data, size_t size) const {
        static_cast<ConcreteSink *>(this)->sendImpl(msgType, data, size);
    }
};

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
 * @brief The Logger class
 *
 * Main logging class. Uses DataProvider implemented by user to get platform-specific data.
 */
template <typename ConfigTag = Config::Traits<Config::Default>,
          typename TDataProvider = DefaultDataProvider,
          typename... TSinkTypes>
class Logger {
    using TConfig = Log::Config::Traits<ConfigTag>;

public:
    using CallbackType = std::function<void(const level, const char *, size_t)>;

    explicit Logger(const TDataProvider &provider, TSinkTypes... sink_args) noexcept
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

    void setUserHandler(CallbackType _handler) { userHandler = _handler; }

    template <typename... Args>
    void fatal(const char *str,
               size_t str_len,
               const std::string_view &file,
               const std::string_view &func,
               const size_t line) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::FatalMsg)) {
                LogRecord record{level::FatalMsg, file, func, line};
                log(record, str, str_len);
            }
        }
    }

    template <typename... Args>
    void error(const char *str,
               size_t str_len,
               const std::string_view &file,
               const std::string_view &func,
               const size_t line) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::ErrorMsg)) {
                LogRecord record{level::ErrorMsg, file, func, line};
                log(record, str, str_len);
            }
        }
    }

    template <typename... Args>
    void warning(const char *str,
                 size_t str_len,
                 const std::string_view &file,
                 const std::string_view &func,
                 const size_t line) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::WarningMsg)) {
                LogRecord record{level::WarningMsg, file, func, line};
                log(record, str, str_len);
            }
        }
    }

    template <typename... Args>
    void info(const char *str,
              size_t str_len,
              const std::string_view &file,
              const std::string_view &func,
              const size_t line) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::InfoMsg)) {
                LogRecord record{level::InfoMsg, file, func, line};
                log(record, str, str_len);
            }
        }
    }

    template <typename... Args>
    void debug(const char *str,
               size_t str_len,
               const std::string_view &file,
               const std::string_view &func,
               const size_t line) const {
        if constexpr (TConfig::FATAL_ENABLED) {
            if (logLevel > static_cast<int>(level::DebugMsg)) {
                LogRecord record{level::DebugMsg, file, func, line};
                log(record, str, str_len);
            }
        }
    }

    /**
     * @brief log
     * @param record holds logging context
     * @param str input string to print in log message
     *
     * Main logging function. Calls provided sinks and callbacks if enabled in `logger_config.h`.
     * All logging calls can be disabled in the same file.
     */
    void log(const LogRecord &record, const char *str, size_t str_len) const {
        // if (static_cast<int>(record.msgType) > logLevel) {
        //     return;
        // }

        std::array<char, TConfig::LOGGER_MAX_STR_SIZE> msg;
        size_t msg_size =
            createMessage(msg.data(), TConfig::LOGGER_MAX_STR_SIZE, record, str, str_len);

        if constexpr (TConfig::ENABLE_SINKS) {
            send_to_all_sinks(record.msgType, msg.data(), msg_size);
        }

        if constexpr (TConfig::ENABLE_PRINT_CALLBACK) {
            if (userHandler != nullptr) {
                userHandler(record.msgType, msg.data(), msg_size);
            }
        }
    }

    /**
     * @brief createMessage
     * @param outBuf poitner to output buffer
     * @param bufSize max size of output buffer
     * @param record holds logging context
     * @param str input string to print in log message
     * @return length of formatted log message
     *
     * Creates log message with specified in @brief setMessagePattern
     * view and put it into `outBuf` and control it's size with `bufSize`.
     */
    size_t createMessage(char *outBuf,
                         size_t bufSize,
                         const LogRecord &record,
                         const char *str,
                         size_t str_len) const {
        size_t pos = 0;

        for (size_t i = 0; i < tokenOpsCount; i++) {
            append(pos, outBuf, bufSize, tokenOps[i].literal, tokenOps[i].literal_len);
            switch (tokenOps[i].type) {
                case tokType::TokDate:
                    tokDateHandler(pos, outBuf, bufSize, record, str, str_len,
                                   data_provider_instance);
                    break;
                case tokType::TokTime:
                    tokTimeHandler(pos, outBuf, bufSize, record, str, str_len,
                                   data_provider_instance);
                    break;
                case tokType::TokLevel:
                    tokLevelHandler(pos, outBuf, bufSize, record, str, str_len,
                                    data_provider_instance);
                    break;
                case tokType::TokFile:
                    tokFileHandler(pos, outBuf, bufSize, record, str, str_len,
                                   data_provider_instance);
                    break;
                case tokType::TokThread:
                    tokThreadHandler(pos, outBuf, bufSize, record, str, str_len,
                                     data_provider_instance);
                    break;
                case tokType::TokFunc:
                    tokFuncHandler(pos, outBuf, bufSize, record, str, str_len,
                                   data_provider_instance);
                    break;
                case tokType::TokLine:
                    tokLineHandler(pos, outBuf, bufSize, record, str, str_len,
                                   data_provider_instance);
                    break;
                case tokType::TokPid:
                    tokPidHandler(pos, outBuf, bufSize, record, str, str_len,
                                  data_provider_instance);
                    break;
                case tokType::TokMessage:
                    tokMessageHandler(pos, outBuf, bufSize, record, str, str_len,
                                      data_provider_instance);
                    break;
                case tokType::TokInvalid:
                    tokInvalidHandler(pos, outBuf, bufSize, record, str, str_len,
                                      data_provider_instance);
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
     * Places string in buffer in needed position and increase `pos` to `dataLen` if data is placed.
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
                               [[maybe_unused]] const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] size_t str_len,
                               const TDataProvider &data_provider_instance) {
        pos += data_provider_instance.getCurrentDate(outBuf + pos, bufSize - pos);
    }

    static void tokTimeHandler(size_t &pos,
                               char *outBuf,
                               [[maybe_unused]] size_t bufSize,
                               [[maybe_unused]] const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] size_t str_len,
                               const TDataProvider &data_provider_instance) {
        pos += data_provider_instance.getCurrentTime(outBuf + pos, bufSize - pos);
    }

    static void tokThreadHandler(size_t &pos,
                                 char *outBuf,
                                 [[maybe_unused]] size_t bufSize,
                                 [[maybe_unused]] const LogRecord &record,
                                 [[maybe_unused]] const char *str,
                                 [[maybe_unused]] size_t str_len,
                                 const TDataProvider &data_provider_instance) {
        pos += data_provider_instance.getThreadId(outBuf + pos, bufSize - pos);
    }

    static void tokPidHandler(size_t &pos,
                              char *outBuf,
                              [[maybe_unused]] size_t bufSize,
                              [[maybe_unused]] const LogRecord &record,
                              [[maybe_unused]] const char *str,
                              [[maybe_unused]] size_t str_len,
                              const TDataProvider &data_provider_instance) {
        pos += data_provider_instance.getProcessName(outBuf + pos, bufSize - pos);
    }

    static void tokLevelHandler(size_t &pos,
                                char *outBuf,
                                size_t bufSize,
                                const LogRecord &record,
                                [[maybe_unused]] const char *str,
                                [[maybe_unused]] size_t str_len,
                                [[maybe_unused]] const TDataProvider &data_provider_instance) {
        const char *ch = msg_log_types[static_cast<int>(record.msgType)].data();
        size_t len = msg_log_types[static_cast<int>(record.msgType)].size();

        append(pos, outBuf, bufSize, ch, len);
    }

    static void tokFileHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] size_t str_len,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, record.file.data(), record.file.size());
    }

    static void tokFuncHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] size_t str_len,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, record.func.data(), record.func.size());
    }

    static void tokLineHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] size_t str_len,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        char *tail = outBuf + pos;
        std::to_chars_result result = std::to_chars(tail, tail + (bufSize - pos), record.line);
        pos += result.ptr - tail;
    }

    static void tokMessageHandler(size_t &pos,
                                  char *outBuf,
                                  size_t bufSize,
                                  [[maybe_unused]] const LogRecord &record,
                                  const char *str,
                                  size_t str_len,
                                  [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, str, str_len);
    }

    static void tokInvalidHandler([[maybe_unused]] size_t &pos,
                                  [[maybe_unused]] char *outBuf,
                                  [[maybe_unused]] size_t bufSize,
                                  [[maybe_unused]] const LogRecord &record,
                                  [[maybe_unused]] const char *str,
                                  [[maybe_unused]] size_t str_len,
                                  [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, "invalid token", sizeof("invalid token"));
    }

    int logLevel = 3;
    /// holds all text before tokens found in `setLogPattern`. Token itself holds legth and pointer
    /// to text that comes before him, @see TokenOp
    std::array<char, TConfig::LOGGER_LITERAL_BUFFER_SIZE> literalBuffer = {};
    /// found tokens, so the output will look the same as `setLogPattern`
    std::array<TokenOp, TConfig::LOGGER_MAX_TOKENS> tokenOps = {};
    /// number of found tokens
    size_t tokenOpsCount = 0;

    /// class that provides platform-dependent data
    TDataProvider data_provider_instance;
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
