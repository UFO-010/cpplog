#ifndef LOGGER_H
#define LOGGER_H

#include <cstring>
#include <charconv>
#include <tuple>
#include <iterator>

#include "logger_config.h"

#if defined(__GNUC__) || defined(__clang__)
    #define LOG_CURRENT_FUNC __PRETTY_FUNCTION__
#elif _MSC_VER && !__INTEL_COMPILER
    #define LOG_CURRENT_FUNC __FUNCSIG__
#else
    #define LOG_CURRENT_FUNC __func__
#endif

#define Debug(LoggerType, message)                                                                \
    if constexpr (LOGGER_LOG_DEBUG_ENABLED) {                                                     \
        LoggerType.log(Log::LogRecord{Log::messageType::DebugMsg, __FILE__, sizeof(__FILE__) - 1, \
                                      LOG_CURRENT_FUNC, sizeof(LOG_CURRENT_FUNC) - 1, __LINE__},  \
                       message);                                                                  \
    }
#define Info(LoggerType, message)                                                                \
    if constexpr (LOGGER_LOG_INFO_ENABLED) {                                                     \
        LoggerType.log(Log::LogRecord{Log::messageType::InfoMsg, __FILE__, sizeof(__FILE__) - 1, \
                                      LOG_CURRENT_FUNC, sizeof(LOG_CURRENT_FUNC) - 1, __LINE__}, \
                       message);                                                                 \
    }
#define Warning(LoggerType, message)                                                     \
    if constexpr (LOGGER_LOG_WARNING_ENABLED) {                                          \
        LoggerType.log(                                                                  \
            Log::LogRecord{Log::messageType::WarningMsg, __FILE__, sizeof(__FILE__) - 1, \
                           LOG_CURRENT_FUNC, sizeof(LOG_CURRENT_FUNC) - 1, __LINE__},    \
            message);                                                                    \
    }
#define Error(LoggerType, message)                                                                \
    if constexpr (LOGGER_LOG_ERROR_ENABLED) {                                                     \
        LoggerType.log(Log::LogRecord{Log::messageType::ErrorMsg, __FILE__, sizeof(__FILE__) - 1, \
                                      LOG_CURRENT_FUNC, sizeof(LOG_CURRENT_FUNC) - 1, __LINE__},  \
                       message);                                                                  \
    }
#define Fatal(LoggerType, message)                                                                \
    if constexpr (LOGGER_LOG_FATAL_ENABLED) {                                                     \
        LoggerType.log(Log::LogRecord{Log::messageType::FatalMsg, __FILE__, sizeof(__FILE__) - 1, \
                                      LOG_CURRENT_FUNC, sizeof(LOG_CURRENT_FUNC) - 1, __LINE__},  \
                       message);                                                                  \
    }

namespace Log {

enum messageType : int {
    FatalMsg,
    ErrorMsg,
    WarningMsg,
    InfoMsg,
    DebugMsg,
};

template <typename ConcreteSink>
class ILogSink {
public:
    void send(const messageType msgType, const char *data, size_t size) const {
        static_cast<ConcreteSink *>(this)->sendImpl(msgType, data, size);
    }
};

struct LogRecord {
public:
    const messageType msgType;
    const char *file;
    const size_t file_len;
    const char *func;
    const size_t func_len;
    const int line;

    constexpr LogRecord(const messageType v_msgType,
                        const char *v_file,
                        size_t v_file_len,
                        const char *v_func,
                        size_t v_func_len,
                        int v_line) noexcept
        : msgType(v_msgType),
          file(v_file),
          file_len(v_file_len),
          func(v_func),
          func_len(v_func_len),
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
    explicit Logger(const TDataProvider &provider = TDataProvider{}, TSinkTypes... sink_args)
        : data_provider_instance(provider),
          sinks_tuple(sink_args...) {
        setLogPattern("%{type}: %{message}");  // default pattern
    }

    /**
     * @brief setLogLevel
     * @param level priority of messages to display
     * Set logging level. Messages with a lower priority level will be ignored
     */
    void setLogLevel(int level) { logLevel = level; }

    int getLevel() const { return logLevel; }

    /**
     * @brief setLogPattern
     * @param pattern Output message pattern
     * Define ouput logging message format to look like.
     * Options : "%{date}"; "%{time}"; "%{type}"; "%{file}"; "%{thread}";
     * "%{function}"; "%{line}"; "%{pid}"; "%{message}".
     * Always put separators between options
     * @example "%{date} %{time}"
     * Output: "<current date> <current time>"
     * @example "%{date}%{time}"
     * Output: "<current date>%{time}"
     */
    bool setLogPattern(const char *pattern) {
        tokenOpsCount = 0;
        size_t literal_buffer_pos = 0;

        const char *p = pattern;
        const char *start_of_literal = p;

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

            size_t literal_len = static_cast<size_t>(token_start - start_of_literal);

            if (literal_buffer_pos + literal_len > LOGGER_LITERAL_BUFFER_SIZE) {
                literal_len = LOGGER_LITERAL_BUFFER_SIZE - literal_buffer_pos;
            }

            char *dest = literalBuffer + literal_buffer_pos;
            if (literal_len > 0) {
                std::memcpy(dest, start_of_literal, literal_len);
            }
            literal_buffer_pos += literal_len;

            size_t token_len = static_cast<size_t>(brace_end + 1 - token_start);
            tokType found_type = tokType::TokInvalid;
            for (size_t i = 0; i < tokensNumber; ++i) {
                if (strncmp(token_start, tokens[i], token_len) == 0) {
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

    void setUserHandler(void (*_handler)(const messageType msgType,
                                         const char *message,
                                         size_t msg_size)) {
        userHandler = _handler;
    }

    /**
     * @brief log
     * @param msgType type of message, see @brief messageType
     * @param file name of file function called from
     * @param func name of function this function called from
     * @param line number of function this function called from
     * @param str input string to print in log message
     *
     * Main logging function. Calls provided sinks and callbacks if enabled. Be careful, function
     * itself don't control logging level.
     */
    void log(const LogRecord &record, const char *str) {
        if (record.msgType > logLevel) {
            return;
        }

        static thread_local char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(msg, sizeof(msg), record, str);

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
     * @param msgType type of message, see @brief messageType
     * @param file name of file function called from
     * @param func name of function this function called from
     * @param line number of function this function called from
     * @param str input string to print in log message
     * @param outBuf poitner to output buffer
     * @param bufSize max size of buffer
     *
     * Creates log message with specified in @brief setMessagePattern
     * view and put it into `outBuf` and control it's size with `bufSize`.
     */
    size_t createMessage(char *outBuf, size_t bufSize, const LogRecord &record, const char *str) {
        size_t pos = 0;

        for (size_t i = 0; i < tokenOpsCount; i++) {
            append(pos, outBuf, bufSize, tokenOps[i].literal, tokenOps[i].literal_len);
            tokenOps[i].handler(pos, outBuf, bufSize, record, str, data_provider_instance);
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
                                    const TDataProvider &data_provider_instance);

    struct TokenOp {
        tokType type;
        const char *literal;
        size_t literal_len;
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
                               size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               const TDataProvider &data_provider_instance) {
        char temp[LOGGER_MAX_TEMP_SIZE];
        size_t len = data_provider_instance.getCurrentDate(temp, LOGGER_MAX_TEMP_SIZE);
        append(pos, outBuf, bufSize, temp, len);
    }

    static void tokTimeHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               const TDataProvider &data_provider_instance) {
        char temp[LOGGER_MAX_TEMP_SIZE];
        size_t len = data_provider_instance.getCurrentTime(temp, LOGGER_MAX_TEMP_SIZE);
        append(pos, outBuf, bufSize, temp, len);
    }

    static void tokThreadHandler(size_t &pos,
                                 char *outBuf,
                                 size_t bufSize,
                                 const LogRecord &record,
                                 [[maybe_unused]] const char *str,
                                 const TDataProvider &data_provider_instance) {
        char temp[LOGGER_MAX_TEMP_SIZE];
        size_t len = data_provider_instance.getThreadId(temp, LOGGER_MAX_TEMP_SIZE);
        append(pos, outBuf, bufSize, temp, len);
    }

    static void tokPidHandler(size_t &pos,
                              char *outBuf,
                              size_t bufSize,
                              const LogRecord &record,
                              [[maybe_unused]] const char *str,
                              const TDataProvider &data_provider_instance) {
        char temp[LOGGER_MAX_TEMP_SIZE];
        size_t len = data_provider_instance.getProcessName(temp, LOGGER_MAX_TEMP_SIZE);
        append(pos, outBuf, bufSize, temp, len);
    }

    static void tokLevelHandler(size_t &pos,
                                char *outBuf,
                                size_t bufSize,
                                const LogRecord &record,
                                [[maybe_unused]] const char *str,
                                [[maybe_unused]] const TDataProvider &data_provider_instance) {
        const char *ch = msg_log_types[static_cast<int>(record.msgType)];
        size_t len = msg_log_type_lengths[static_cast<int>(record.msgType)];

        append(pos, outBuf, bufSize, ch, len);
    }

    static void tokFileHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, record.file, record.file_len);
    }

    static void tokFuncHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, record.func, record.func_len);
    }

    static void tokLineHandler(size_t &pos,
                               char *outBuf,
                               [[maybe_unused]] size_t bufSize,
                               const LogRecord &record,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        char *tail = outBuf + pos;
        std::to_chars_result result = std::to_chars(tail, tail + (bufSize - pos), record.line);
        pos += result.ptr - tail;
    }

    static void tokMessageHandler(size_t &pos,
                                  char *outBuf,
                                  size_t bufSize,
                                  const LogRecord &record,
                                  [[maybe_unused]] const char *str,
                                  [[maybe_unused]] const TDataProvider &data_provider_instance) {
        size_t len = strnlen(str, LOGGER_MAX_MESSAGE_SIZE);
        append(pos, outBuf, bufSize, str, len);
    }

    static void tokInvalidHandler([[maybe_unused]] size_t &pos,
                                  [[maybe_unused]] char *outBuf,
                                  [[maybe_unused]] size_t bufSize,
                                  const LogRecord &record,
                                  [[maybe_unused]] const char *str,
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
    /// holds messages that placed after tokens passed in  @brief setMessagePatter
    char literalBuffer[LOGGER_LITERAL_BUFFER_SIZE] = {};
    /// holds pointers to tokens, so the output will look the  same as @brief setMessagePattern
    TokenOp tokenOps[LOGGER_MAX_TOKENS] = {};
    /// holds number of found tokens
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
        [[maybe_unused]] const messageType &msgType,
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
        I<sizeof...(TSinkTypes), void>::type send_to_all_sinks(const messageType &msgType,
                                                               const char *data,
                                                               size_t size) const {
        // call current sink
        std::get<I>(sinks_tuple).send(msgType, data, size);
        // call next sink
        send_to_all_sinks<I + 1>(msgType, data, size);
    }

    /// user callback to print logging message
    void (*userHandler)(const messageType msgType, const char *message, size_t msg_size) = nullptr;

    /// types of logging level, added to output message
    static constexpr const char *msg_log_types[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};
    /// sizes of logging level string, hardcoded
    static constexpr size_t msg_log_type_lengths[] = {5, 5, 4, 4, 5};

    /// tokens for message pattern
    static constexpr const char *tokens[] = {"%{date}", "%{time}",   "%{type}",
                                             "%{file}", "%{thread}", "%{function}",
                                             "%{line}", "%{pid}",    "%{message}"};
    static constexpr size_t tokensNumber = std::size(tokens);
};

}  // namespace Log

#endif
