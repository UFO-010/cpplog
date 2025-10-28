#ifndef LOGGER_H
#define LOGGER_H

#include <cstring>
#include <charconv>
#include <tuple>
#include <array>
#include <string_view>

#include "logger_config.h"

#if defined(__GNUC__) || defined(__clang__)
    #define LOG_CURRENT_FUNC __PRETTY_FUNCTION__
#elif _MSC_VER && !__INTEL_COMPILER
    #define LOG_CURRENT_FUNC __FUNCSIG__
#else
    #define LOG_CURRENT_FUNC __func__
#endif

#define Debug(LoggerType, message, len)                                                           \
    if constexpr (LOGGER_LOG_DEBUG_ENABLED) {                                                     \
        constexpr std::string_view file_sv = __FILE__;                                            \
        constexpr std::string_view func_sv = LOG_CURRENT_FUNC;                                    \
        LoggerType.log(Log::LogRecord{Log::level::DebugMsg, file_sv, func_sv, __LINE__}, message, \
                       len);                                                                      \
    }
#define Info(LoggerType, message, len)                                                           \
    if constexpr (LOGGER_LOG_INFO_ENABLED) {                                                     \
        constexpr std::string_view file_sv = __FILE__;                                           \
        constexpr std::string_view func_sv = LOG_CURRENT_FUNC;                                   \
        LoggerType.log(Log::LogRecord{Log::level::InfoMsg, file_sv, func_sv, __LINE__}, message, \
                       len);                                                                     \
    }
#define Warning(LoggerType, message, len)                                                  \
    if constexpr (LOGGER_LOG_WARNING_ENABLED) {                                            \
        constexpr std::string_view file_sv = __FILE__;                                     \
        constexpr std::string_view func_sv = LOG_CURRENT_FUNC;                             \
        LoggerType.log(Log::LogRecord{Log::level::WarningMsg, file_sv, func_sv, __LINE__}, \
                       message, len);                                                      \
    }
#define Error(LoggerType, message, len)                                                           \
    if constexpr (LOGGER_LOG_ERROR_ENABLED) {                                                     \
        constexpr std::string_view file_sv = __FILE__;                                            \
        constexpr std::string_view func_sv = LOG_CURRENT_FUNC;                                    \
        LoggerType.log(Log::LogRecord{Log::level::ErrorMsg, file_sv, func_sv, __LINE__}, message, \
                       len);                                                                      \
    }
#define Fatal(LoggerType, message, len)                                                           \
    if constexpr (LOGGER_LOG_FATAL_ENABLED) {                                                     \
        constexpr std::string_view file_sv = __FILE__;                                            \
        constexpr std::string_view func_sv = LOG_CURRENT_FUNC;                                    \
        LoggerType.log(Log::LogRecord{Log::level::FatalMsg, file_sv, func_sv, __LINE__}, message, \
                       len);                                                                      \
    }

namespace Log {

enum class level : int {
    FatalMsg = 0,
    ErrorMsg = 1,
    WarningMsg = 2,
    InfoMsg = 3,
    DebugMsg = 4,
};

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
    const int line;

    constexpr LogRecord(const level v_msgType,
                        const std::string_view v_file,
                        const std::string_view &v_func,
                        int v_line) noexcept
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
template <typename TDataProvider, typename... TSinkTypes>
class Logger {
public:
    explicit Logger(const TDataProvider &provider = TDataProvider{},
                    TSinkTypes... sink_args) noexcept
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

        while (*p != '\0' && tokenOpsCount < LOGGER_MAX_TOKENS) {
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

            if (literal_buffer_pos + literal_len > LOGGER_LITERAL_BUFFER_SIZE) {
                literal_len = LOGGER_LITERAL_BUFFER_SIZE - literal_buffer_pos;
            }

            if (literal_buffer_pos >= LOGGER_LITERAL_BUFFER_SIZE) {
                break;
            }

            char *dest = literal + literal_buffer_pos;
            if (literal_len > 0) {
                std::memcpy(dest, start_of_literal, literal_len);
            }
            literal_buffer_pos += literal_len;

            auto token_len = static_cast<size_t>(brace_end + 1 - token_start);
            tokType found_type = tokType::TokInvalid;

            for (size_t i = 0; i < tokens.size(); ++i) {
                if (strncmp(token_start, tokens[i].data(), token_len) == 0) {
                    found_type = static_cast<tokType>(i);
                    break;
                }
            }

            TokHandlerFunc handler = getTokHandler(found_type);

            if (handler == nullptr) {
                return false;
            }

            tokenOps[tokenOpsCount] = {found_type, dest, literal_len, handler};
            ++tokenOpsCount;
            p = brace_end + 1;
            start_of_literal = p;
        }
        return true;
    }

    void setUserHandler(void (*_handler)(const level msgType,
                                         const char *message,
                                         size_t msg_size)) {
        userHandler = _handler;
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
        if (static_cast<int>(record.msgType) > logLevel) {
            return;
        }

        static thread_local char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(msg, LOGGER_MAX_STR_SIZE, record, str, str_len);

        if constexpr (ENABLE_SINKS) {
            send_to_all_sinks(record.msgType, msg, msg_size);
        }

        if constexpr (ENABLE_PRINT_CALLBACK) {
            if (userHandler != nullptr) {
                userHandler(record.msgType, msg, msg_size);
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

    using TokHandlerFunc = void (*)(size_t &pos,
                                    char *outBuf,
                                    size_t bufSize,
                                    const LogRecord &record,
                                    const char *str,
                                    size_t str_len,
                                    const TDataProvider &data_provider_instance);

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
        /// function pointer fallback, currently unused
        TokHandlerFunc handler;
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

    TokHandlerFunc getTokHandler(const tokType &type) {
        TokHandlerFunc handler = nullptr;
        switch (type) {
            case tokType::TokDate:
                handler = tokDateHandler;
                break;
            case tokType::TokTime:
                handler = tokTimeHandler;
                break;
            case tokType::TokLevel:
                handler = tokLevelHandler;
                break;
            case tokType::TokFile:
                handler = tokFileHandler;
                break;
            case tokType::TokThread:
                handler = tokThreadHandler;
                break;
            case tokType::TokFunc:
                handler = tokFuncHandler;
                break;
            case tokType::TokLine:
                handler = tokLineHandler;
                break;
            case tokType::TokPid:
                handler = tokPidHandler;
                break;
            case tokType::TokMessage:
                handler = tokMessageHandler;
                break;
            case tokType::TokInvalid:
                handler = tokInvalidHandler;
                break;
            default:
                break;
        }
        return handler;
    }

    int logLevel = 3;
    /// holds all text before tokens found in `setLogPattern`. Token itself holds legth and pointer
    /// to text that comes before him, @see TokenOp
    std::array<char, LOGGER_LITERAL_BUFFER_SIZE> literalBuffer = {};
    /// found tokens, so the output will look the same as `setLogPattern`
    std::array<TokenOp, LOGGER_MAX_TOKENS> tokenOps = {};
    /// number of found tokens
    size_t tokenOpsCount = 0;

    /// class that provides platform-dependent data
    TDataProvider data_provider_instance;
    /// holds sinks to send log messages to
    std::tuple<TSinkTypes...> sinks_tuple;

    /**
     * @brief send_to_all_sinks
     * @param msgType
     * @param data
     * @param size
     * @return
     *
     * No sinks left, do nothing
     */
    template <std::size_t I = 0>
    inline typename std::enable_if<I == sizeof...(TSinkTypes), void>::type send_to_all_sinks(
        [[maybe_unused]] const level &msgType,
        [[maybe_unused]] const char *data,
        [[maybe_unused]] size_t size) const {
        // no sinks left, do nothing
    }

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
        inline typename std::enable_if <
        I<sizeof...(TSinkTypes), void>::type send_to_all_sinks(const level &msgType,
                                                               const char *data,
                                                               size_t size) const {
        // call current sink
        std::get<I>(sinks_tuple).send(msgType, data, size);
        // call next sink
        send_to_all_sinks<I + 1>(msgType, data, size);
    }

    /// user callback to print logging message
    void (*userHandler)(const level msgType, const char *message, size_t msg_size) = nullptr;

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
