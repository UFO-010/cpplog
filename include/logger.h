#ifndef LOGGER_H
#define LOGGER_H

#include <cstring>
#include <charconv>
#include <stdlib.h>

#include "logger_config.h"

#if defined(__GNUC__) || defined(__clang__)
    #define LOG_CURRENT_FUNC __PRETTY_FUNCTION__
#elif _MSC_VER && !__INTEL_COMPILER
    #define LOG_CURRENT_FUNC __FUNCSIG__
#else
    #define LOG_CURRENT_FUNC __func__
#endif

#define Debug(message)                                                                  \
    if constexpr (LOGGER_LOG_DEBUG_ENABLED) {                                           \
        log(Log::messageType::DebugMsg, __FILE__, LOG_CURRENT_FUNC, __LINE__, message); \
    }
#define Info(LoggerType, message)                                                      \
    if constexpr (LOGGER_LOG_INFO_ENABLED) {                                           \
        log(Log::messageType::InfoMsg, __FILE__, LOG_CURRENT_FUNC, __LINE__, message); \
    }
#define Warning(LoggerType, message)                                                      \
    if constexpr (LOGGER_LOG_WARNING_ENABLED) {                                           \
        log(Log::messageType::WarningMsg, __FILE__, LOG_CURRENT_FUNC, __LINE__, message); \
    }
#define Error(LoggerType, message)                                                      \
    if constexpr (LOGGER_LOG_ERROR_ENABLED) {                                           \
        log(Log::messageType::ErrorMsg, __FILE__, LOG_CURRENT_FUNC, __LINE__, message); \
    }
#define Fatal(LoggerType, message)                                                      \
    if constexpr (LOGGER_LOG_FATAL_ENABLED) {                                           \
        log(Log::messageType::FatalMsg, __FILE__, LOG_CURRENT_FUNC, __LINE__, message); \
    }

namespace Log {

enum messageType {
    FatalMsg,
    ErrorMsg,
    WarningMsg,
    InfoMsg,
    DebugMsg,
};

class ILogSink {
public:
    virtual void send(const messageType &msgType, const char *data, size_t size) = 0;
};

class DataProvider {
public:
    virtual const char *getProcessName(char *buffer, size_t bufferSize) const = 0;
    virtual const char *getThreadId(char *buffer, size_t bufferSize) const = 0;
    virtual const char *getCurrentDate(char *buffer, size_t bufferSize) const = 0;
    virtual const char *getCurrentTime(char *buffer, size_t bufferSize) const = 0;
};

/**
 * @brief The Logger class
 *
 * Main logging class. Uses DataProvider implemented by user to get platform-specific data. Be
 * careful, object lifetime should be controlled by user.
 */
template <typename TDataProvider>
class Logger {
public:
    Logger(const TDataProvider &provider = TDataProvider{})
        : data_provider_instance(provider),
          logLevel(3),
          userHandler(nullptr) {}

    /**
     * @brief setLogLevel
     * @param level priority of messages to display
     * Set logging level. Messages with a lower priority level will be ignored
     */
    void setLogLevel(int level) { logLevel = level; }

    int getLevel() { return logLevel; }

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
    void setLogPattern(const char *pattern) {
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
            if (brace_end == NULL) {
                break;
            }

            size_t literal_len = static_cast<size_t>(token_start - start_of_literal);

            if (literal_buffer_pos + literal_len > LOGGER_LITERAL_BUFFER_SIZE) {
                literal_len = LOGGER_LITERAL_BUFFER_SIZE - literal_buffer_pos;
            }

            char *dest = literalBuffer + literal_buffer_pos;
            if (literal_len > 0) {
                memcpy(dest, start_of_literal, literal_len);
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

            if (found_type != tokType::TokInvalid) {
                tokenOps[tokenOpsCount] = {found_type, dest, literal_len};
                ++tokenOpsCount;
                p = brace_end + 1;
                start_of_literal = p;
            } else {
                ++p;
            }
        }

        // size_t tail_len = strlen(start_of_literal);
        // if (literal_buffer_pos + tail_len > LOGGER_LITERAL_BUFFER_SIZE) {
        //     tail_len = LOGGER_LITERAL_BUFFER_SIZE - literal_buffer_pos;
        // }
        // char *pattern_tail = literal_buffer + literal_buffer_pos;
        // if (tail_len > 0) {
        //     memcpy(const_cast<char *>(pattern_tail), start_of_literal, tail_len);
        // }
        // size_t pattern_tail_len = tail_len;
    }

    void setUserHandler(void (*_handler)(const messageType &msgType,
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
    void log(
        const messageType &msgType, const char *file, const char *func, int line, const char *str) {
        if (msgType > logLevel) {
            return;
        }

        static thread_local char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(msgType, file, func, line, str, msg, sizeof(msg));

#if ENABLE_SINKS == 1
        for (int i = 0; i < sinkCount; i++) {
            sinks[i]->send(msgType, msg, msg_size);
        }
#endif
#if ENABLE_PRINT_CALLBACK == 1
        if (user_handler != nullptr) {
            user_handler(msgType, msg, msg_size);
        }
#endif
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
    size_t createMessage(const messageType &msgType,
                         const char *file,
                         const char *func,
                         const int line,
                         const char *str,
                         char *outBuf,
                         size_t bufSize) {
        size_t pos = 0;

        char temp[LOGGER_MAX_TEMP_SIZE];
        for (size_t i = 0; i < tokenOpsCount; i++) {
            append(pos, outBuf, bufSize, tokenOps[i].literal, tokenOps[i].literal_len);
            const int handerPos = static_cast<int>(tokenOps[i].type);
            tokHanlders[handerPos](pos, outBuf, bufSize, msgType, file, func, line, str, temp,
                                   LOGGER_MAX_TEMP_SIZE, data_provider_instance);
        }

        // if (pos < bufSize) {
        outBuf[pos] = '\0';
        // } else if (bufSize != 0) {
        //     outBuf[bufSize - 1] = '\0';
        //     pos = bufSize - 1;
        // }

        return pos;
    }

    void addSink(ILogSink *sink) {
        if (sinkCount < LOGGER_MAX_SINKS) {
            sinks[sinkCount] = sink;
            sinkCount++;
        }
    }

private:
    enum class tokType {
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

    struct TokenOp {
        tokType type;
        const char *literal;
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
     * Places string in buffer in needed position
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
                               [[maybe_unused]] const Log::messageType msgType,
                               [[maybe_unused]] const char *file,
                               [[maybe_unused]] const char *func,
                               [[maybe_unused]] int line,
                               [[maybe_unused]] const char *str,
                               char *temp,
                               size_t temp_size,
                               const TDataProvider &data_provider_instance) {
        data_provider_instance.getCurrentDate(temp, temp_size);
        append(pos, outBuf, bufSize, temp, std::strlen(temp));
    }

    static void tokTimeHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               [[maybe_unused]] const Log::messageType msgType,
                               [[maybe_unused]] const char *file,
                               [[maybe_unused]] const char *func,
                               [[maybe_unused]] int line,
                               [[maybe_unused]] const char *str,
                               char *temp,
                               size_t temp_size,
                               const TDataProvider &data_provider_instance) {
        data_provider_instance.getCurrentTime(temp, temp_size);
        append(pos, outBuf, bufSize, temp, std::strlen(temp));
    }

    static void tokLevelHandler(size_t &pos,
                                char *outBuf,
                                size_t bufSize,
                                [[maybe_unused]] const Log::messageType msgType,
                                [[maybe_unused]] const char *file,
                                [[maybe_unused]] const char *func,
                                [[maybe_unused]] int line,
                                [[maybe_unused]] const char *str,
                                [[maybe_unused]] char *temp,
                                [[maybe_unused]] size_t temp_size,
                                [[maybe_unused]] const TDataProvider &data_provider_instance) {
        const char *ch = msg_log_types[static_cast<int>(msgType)];
        append(pos, outBuf, bufSize, ch, std::strlen(ch));
    }

    static void tokFileHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               [[maybe_unused]] const Log::messageType msgType,
                               [[maybe_unused]] const char *file,
                               [[maybe_unused]] const char *func,
                               [[maybe_unused]] int line,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] char *temp,
                               [[maybe_unused]] size_t temp_size,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, file, std::strlen(file));
    }

    static void tokThreadHandler(size_t &pos,
                                 char *outBuf,
                                 size_t bufSize,
                                 [[maybe_unused]] const Log::messageType msgType,
                                 [[maybe_unused]] const char *file,
                                 [[maybe_unused]] const char *func,
                                 [[maybe_unused]] int line,
                                 [[maybe_unused]] const char *str,
                                 char *temp,
                                 size_t temp_size,
                                 const TDataProvider &data_provider_instance) {
        data_provider_instance.getThreadId(temp, temp_size);
        append(pos, outBuf, bufSize, temp, std::strlen(temp));
    }

    static void tokFuncHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               [[maybe_unused]] const Log::messageType msgType,
                               [[maybe_unused]] const char *file,
                               [[maybe_unused]] const char *func,
                               [[maybe_unused]] int line,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] char *temp,
                               [[maybe_unused]] size_t temp_size,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, func, std::strlen(func));
    }

    static void tokLineHandler(size_t &pos,
                               char *outBuf,
                               [[maybe_unused]] size_t bufSize,
                               [[maybe_unused]] const Log::messageType msgType,
                               [[maybe_unused]] const char *file,
                               [[maybe_unused]] const char *func,
                               [[maybe_unused]] int line,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] char *temp,
                               [[maybe_unused]] size_t temp_size,
                               [[maybe_unused]] const TDataProvider &data_provider_instance) {
        char *tail = outBuf + pos;
        std::to_chars_result result = std::to_chars(tail, tail + (bufSize - pos), line);
        pos += result.ptr - tail;
    }

    static void tokPidHandler(size_t &pos,
                              char *outBuf,
                              size_t bufSize,
                              [[maybe_unused]] const Log::messageType msgType,
                              [[maybe_unused]] const char *file,
                              [[maybe_unused]] const char *func,
                              [[maybe_unused]] int line,
                              [[maybe_unused]] const char *str,
                              char *temp,
                              size_t temp_size,
                              const TDataProvider &data_provider_instance) {
        data_provider_instance.getProcessName(temp, temp_size);
        append(pos, outBuf, bufSize, temp, std::strlen(temp));
    }

    static void tokMessageHandler(size_t &pos,
                                  char *outBuf,
                                  size_t bufSize,
                                  [[maybe_unused]] const Log::messageType msgType,
                                  [[maybe_unused]] const char *file,
                                  [[maybe_unused]] const char *func,
                                  [[maybe_unused]] int line,
                                  [[maybe_unused]] const char *str,
                                  [[maybe_unused]] char *temp,
                                  [[maybe_unused]] size_t temp_size,
                                  [[maybe_unused]] const TDataProvider &data_provider_instance) {
        append(pos, outBuf, bufSize, str, std::strlen(str));
    }

    static void tokInvalidHandler([[maybe_unused]] size_t &pos,
                                  [[maybe_unused]] char *outBuf,
                                  [[maybe_unused]] size_t bufSize,
                                  [[maybe_unused]] const Log::messageType msgType,
                                  [[maybe_unused]] const char *file,
                                  [[maybe_unused]] const char *func,
                                  [[maybe_unused]] int line,
                                  [[maybe_unused]] const char *str,
                                  [[maybe_unused]] char *temp,
                                  [[maybe_unused]] size_t temp_size,
                                  [[maybe_unused]] const TDataProvider &data_provider_instance) {}

    /// class that provides platform-dependent data
    TDataProvider data_provider_instance;
    int logLevel;
    ILogSink *sinks[LOGGER_MAX_SINKS];
    int sinkCount;

    /// callback to print logging message
    void (*userHandler)(const messageType &msgType, const char *message, size_t msg_size);

    /// holds pointers to tokens, so the output will look the  same as @brief setMessagePattern
    TokenOp tokenOps[LOGGER_MAX_TOKENS];
    /// holds number of found tokens
    size_t tokenOpsCount;
    /// holds messages that placed after tokens passed in  @brief setMessagePatter
    char literalBuffer[LOGGER_LITERAL_BUFFER_SIZE];

    using TokHandlerFunc = void (*)(size_t &pos,
                                    char *outBuf,
                                    size_t bufSize,
                                    const Log::messageType msgType,
                                    const char *file,
                                    const char *func,
                                    int line,
                                    const char *str,
                                    char *temp,
                                    size_t temp_size,
                                    const TDataProvider &data_provider_instance);
    /// Handlers to print data in logging message, handlers should be in the same place as
    /// corresponding tokens in array `tokens`
    static constexpr TokHandlerFunc tokHanlders[] = {
        tokDateHandler, tokTimeHandler, tokLevelHandler, tokFileHandler,   tokThreadHandler,
        tokFuncHandler, tokLineHandler, tokPidHandler,   tokMessageHandler};

    /// types of logging level, added to output message
    static constexpr const char *msg_log_types[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

    /// tokens for message pattern
    static constexpr const char *tokens[] = {"%{date}", "%{time}",   "%{type}",
                                             "%{file}", "%{thread}", "%{function}",
                                             "%{line}", "%{pid}",    "%{message}"};
    static constexpr size_t tokensNumber = sizeof(tokens) / sizeof(tokens[0]);
};

}  // namespace Log

#endif
