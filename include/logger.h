#ifndef LOGGER_H
#define LOGGER_H

#include <cstdio>
#include <cstring>

#include "logger_config.h"

#if defined(__GNUC__) || defined(__clang__)
    #define Debug(message)                                                                    \
        Log::Logger::log(Log::messageType::DebugMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__, \
                         message)
    #define Info(message)                                                                    \
        Log::Logger::log(Log::messageType::InfoMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__, \
                         message)
    #define Warning(message)                                                                    \
        Log::Logger::log(Log::messageType::WarningMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__, \
                         message)
    #define Error(message)                                                                    \
        Log::Logger::log(Log::messageType::ErrorMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__, \
                         message)
    #define Fatal(message)                                                                    \
        Log::Logger::log(Log::messageType::FatalMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__, \
                         message)

#elif _MSC_VER && !__INTEL_COMPILER
    #define Debug(message) \
        Log::Logger::log(Log::messageType::DebugMsg, __FILE__, __FUNCSIG__, __LINE__, message)
    #define Info(message) \
        Log::Logger::log(Log::messageType::InfoMsg, __FILE__, __FUNCSIG__, __LINE__, message)
    #define Warning(message) \
        Log::Logger::log(Log::messageType::WarningMsg, __FILE__, __FUNCSIG__, __LINE__, message)
    #define Error(message) \
        Log::Logger::log(Log::messageType::ErrorMsg, __FILE__, __FUNCSIG__, __LINE__, message)
    #define Fatal(message) \
        Log::Logger::log(Log::messageType::FatalMsg, __FILE__, __FUNCSIG__, __LINE__, message)

#elif
    #define Debug(message) \
        Log::Logger::log(Log::messageType::DebugMsg, __FILE__, __func__, __LINE__, message)
    #define Info(message) \
        Log::Logger::log(Log::messageType::InfoMsg, __FILE__, __func__, __LINE__, message)
    #define Warning(message) \
        Log::Logger::log(Log::messageType::WarningMsg, __FILE__, __func__, __LINE__, message)
    #define Error(message) \
        Log::Logger::log(Log::messageType::ErrorMsg, __FILE__, __func__, __LINE__, message)
    #define Fatal(message) \
        Log::Logger::log(Log::messageType::FatalMsg, __FILE__, __func__, __LINE__, message)

#endif

namespace Log {

enum class messageType : int {
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
 * Main logging class. In common case, it is useful to call the wrappers.
 * Used through macros Debug(), Info(), Warning(), Crirical(), Fatal(),
 * which are expanded to function wrappers @brief log_debug(__FILE__,
 * __PRETTY_FUNCTION__, __LINE__, message), etc.
 */
class Logger {
public:
    /**
     * @brief setLogLevel
     * @param level priority of messages to display
     * Set logging level. Messages with a lower priority level will be ignored
     */
    static void setLogLevel(int level) { logLevel = level; }

    static int getLevel() { return logLevel; }

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
    static void setLogPattern(const char *pattern) {
        tokenOpsCount = 0;
        size_t literal_buffer_pos = 0;

        const char *p = pattern;
        const char *start_of_literal = p;

        while (*p && tokenOpsCount < LOGGER_MAX_TOKENS) {
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
            if (!brace_end) break;

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
            tokenType found_type = TokInvalid;
            for (size_t i = 0; i < tokensNumber; ++i) {
                if (strncmp(token_start, tokens[i], token_len) == 0) {
                    found_type = static_cast<tokenType>(i);
                    break;
                }
            }

            if (found_type != TokInvalid) {
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

    static void setUserHandler(void (*_handler)(const messageType &msgType,
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
     * Main logging function. Calls provided sinks and callbacks if enabled
     */
    static void log(
        const messageType &msgType, const char *file, const char *func, int line, const char *str) {
        if (logLevel < static_cast<int>(msgType)) {
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
    static size_t createMessage(const messageType &msgType,
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
            const int handerPos = static_cast<const int>(tokenOps[i].type);
            tokHanlders[handerPos](pos, outBuf, bufSize, msgType, file, func, line, str, temp,
                                   LOGGER_MAX_TEMP_SIZE);
        }

        if (pos < bufSize) {
            outBuf[pos] = '\0';
        } else if (bufSize != 0) {
            outBuf[bufSize - 1] = '\0';
            pos = bufSize - 1;
        }

        return pos;
    }

    static void addSink(ILogSink *sink) {
        if (sinkCount < LOGGER_MAX_SINKS) {
            sinks[sinkCount] = sink;
            sinkCount++;
        }
    }

    static void setDataProvider(DataProvider *provider) { data_provider = provider; }

private:
    Logger() {}

    enum tokenType {
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
        tokenType type;
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
        size_t cur_pos = pos;
        size_t cnt = 0;

        if (cur_pos + dataLen < bufSize) {
            cnt = dataLen;
        } else if (bufSize > cur_pos) {
            cnt = bufSize - cur_pos - 1;
        } else {
            cnt = 0;
        }
        if (cnt != 0) {
            std::memcpy(outBuf + cur_pos, data, cnt);
            cur_pos += cnt;
        }
        pos = cur_pos;
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
                               size_t temp_size) {
        data_provider->getCurrentDate(temp, temp_size);
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
                               size_t temp_size) {
        data_provider->getCurrentTime(temp, temp_size);
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
                                [[maybe_unused]] size_t temp_size) {
        const char *ch = msg_log_types[static_cast<const int>(msgType)];
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
                               [[maybe_unused]] size_t temp_size) {
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
                                 size_t temp_size) {
        data_provider->getThreadId(temp, temp_size);
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
                               [[maybe_unused]] size_t temp_size) {
        append(pos, outBuf, bufSize, func, std::strlen(func));
    }

    static void tokLineHandler(size_t &pos,
                               char *outBuf,
                               size_t bufSize,
                               [[maybe_unused]] const Log::messageType msgType,
                               [[maybe_unused]] const char *file,
                               [[maybe_unused]] const char *func,
                               [[maybe_unused]] int line,
                               [[maybe_unused]] const char *str,
                               [[maybe_unused]] char *temp,
                               [[maybe_unused]] size_t temp_size) {
        char numbuf[LOGGER_MAX_NUMBUF_SIZE];
        int n = std::snprintf(numbuf, sizeof(numbuf), "%d", line);
        append(pos, outBuf, bufSize, numbuf, static_cast<size_t>(n));
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
                              size_t temp_size) {
        data_provider->getProcessName(temp, temp_size);
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
                                  [[maybe_unused]] size_t temp_size) {
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
                                  [[maybe_unused]] size_t temp_size) {}

    static int logLevel;
    static ILogSink *sinks[LOGGER_MAX_SINKS];
    static int sinkCount;
    /// class that provides platform-dependent data
    static DataProvider *data_provider;
    /// callback to print logging message
    static void (*userHandler)(const messageType &msgType, const char *message, size_t msg_size);

    /// holds pointers to tokens, so the output will look the  same as @brief setMessagePattern
    static TokenOp tokenOps[LOGGER_MAX_TOKENS];
    /// holds number of found tokens
    static size_t tokenOpsCount;
    /// holds messages that placed after tokens passed in  @brief setMessagePatter
    static char literalBuffer[LOGGER_LITERAL_BUFFER_SIZE];

    /// types of logging level, added to output message
    static constexpr const char *msg_log_types[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

    /// tokens for message pattern
    static constexpr const char *tokens[] = {"%{date}", "%{time}",   "%{type}",
                                             "%{file}", "%{thread}", "%{function}",
                                             "%{line}", "%{pid}",    "%{message}"};
    static constexpr size_t tokensNumber = sizeof(tokens) / sizeof(tokens[0]);

    using TokHandlerFunc = void (*)(size_t &pos,
                                    char *outBuf,
                                    size_t bufSize,
                                    const Log::messageType msgType,
                                    const char *file,
                                    const char *func,
                                    int line,
                                    const char *str,
                                    char *temp,
                                    size_t temp_size);
    /// Handlers to print data in logging message, handlers should be in the same place as
    /// corresponding tokens in array `tokens`
    static constexpr TokHandlerFunc tokHanlders[] = {
        tokDateHandler, tokTimeHandler, tokLevelHandler, tokFileHandler,   tokThreadHandler,
        tokFuncHandler, tokLineHandler, tokPidHandler,   tokMessageHandler};
};

}  // namespace Log

#endif
