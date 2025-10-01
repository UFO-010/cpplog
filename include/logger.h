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
     * @brief Logger::setLogLevel
     * @param level priority of messages to display
     * Set logging level. Messages with a lower priority level will be ignored
     */
    static void setLogLevel(int level) { log_level = level; }

    static int getLevel() { return log_level; }

    /**
     * @brief Logger::setLogPattern
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
        token_ops_count = 0;
        size_t literal_buffer_pos = 0;

        const char *p = pattern;
        const char *start_of_literal = p;

        while (*p && token_ops_count < LOGGER_MAX_TOKENS) {
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

            char *dest = literal_buffer + literal_buffer_pos;
            if (literal_len > 0) {
                memcpy(dest, start_of_literal, literal_len);
            }
            literal_buffer_pos += literal_len;

            size_t token_len = static_cast<size_t>(brace_end + 1 - token_start);
            tokenType found_type = TokInvalid;
            for (size_t i = 0; i < num_token_types; ++i) {
                if (strncmp(token_start, tokens[i], token_len) == 0) {
                    found_type = static_cast<tokenType>(i);
                    break;
                }
            }

            if (found_type != TokInvalid) {
                tokenOps[token_ops_count] = {found_type, dest, literal_len};
                ++token_ops_count;
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
        user_handler = _handler;
    }

    static void log(
        const messageType &msgType, const char *file, const char *func, int line, const char *str) {
        if (log_level < static_cast<int>(msgType)) {
            return;
        }

        static thread_local char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(msgType, file, func, line, str, msg, sizeof(msg));

#if ENABLE_SINKS == 1
        for (int i = 0; i < sink_count; i++) {
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
     * @brief Logger::createMessage
     * @param msgType type of message, see @param messageType
     * @param file name of file function called from
     * @param func name of function this function called from
     * @param line number of function this function called from
     * @param str input string to print in log message
     * @param outBuf poitner to output buffer
     * @param bufSize max size of buffer
     *
     * Creates log message with specified in @brief Logger::setMessagePattern
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

        auto append = [&pos, bufSize, outBuf](const char *data, size_t len) {
            size_t cnt = 0;

            if (pos + len < bufSize) {
                cnt = len;
            } else if (bufSize > pos) {
                cnt = bufSize - pos - 1;
            } else {
                cnt = 0;
            }
            if (cnt != 0) {
                std::memcpy(outBuf + pos, data, cnt);
                pos += cnt;
            }
        };

        auto appendC = [append](const char *s) { append(s, std::strlen(s)); };

        auto appendInt = [append](int v) {
            char numbuf[LOGGER_MAX_NUMBUF_SIZE];
            int n = std::snprintf(numbuf, sizeof(numbuf), "%d", v);
            if (n > 0) {
                append(numbuf, static_cast<size_t>(n));
            }
        };

        char temp[LOGGER_MAX_TEMP_SIZE];
        for (size_t i = 0; i < token_ops_count; i++) {
            append(tokenOps[i].literal, tokenOps[i].literal_len);
            switch (tokenOps[i].type) {
                case TokDate:
                    data_provider->getCurrentDate(temp, sizeof(temp));
                    appendC(temp);
                    break;
                case TokTime:
                    data_provider->getCurrentTime(temp, sizeof(temp));
                    appendC(temp);
                    break;
                case TokLevel:
                    appendC(msg_log_types[static_cast<int>(msgType)]);
                    break;
                case TokFile:
                    appendC(file);
                    break;
                case TokThread:
                    data_provider->getThreadId(temp, sizeof(temp));
                    appendC(temp);
                    break;
                case TokFunc:
                    appendC(func);
                    break;
                case TokLine:
                    appendInt(line);
                    break;
                case TokPid:
                    data_provider->getProcessName(temp, sizeof(temp));
                    appendC(temp);
                    break;
                case TokMessage:
                    appendC(str);
                    break;
                case TokInvalid:
                    break;
                default:
                    break;
            }
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
        if (sink_count < LOGGER_MAX_SINKS) {
            sinks[sink_count] = sink;
            sink_count++;
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

    static int log_level;
    static ILogSink *sinks[LOGGER_MAX_SINKS];
    static int sink_count;
    static DataProvider *data_provider;
    static void (*user_handler)(const messageType &msgType, const char *message, size_t msg_size);

    /// holds pointers to tokens, so the output will look the  same as @brief setMessagePattern
    static TokenOp tokenOps[LOGGER_MAX_TOKENS];
    /// holds number of found tokens
    static size_t token_ops_count;
    /// holds messages that placed after tokens passed in  @brief setMessagePatter
    static char literal_buffer[LOGGER_LITERAL_BUFFER_SIZE];

    /// tokens for message pattern
    static constexpr const char *tokens[] = {"%{date}", "%{time}",   "%{type}",
                                             "%{file}", "%{thread}", "%{function}",
                                             "%{line}", "%{pid}",    "%{message}"};
    static constexpr size_t num_token_types = sizeof(tokens) / sizeof(tokens[0]);

    /// types of logging level, added to output message
    static constexpr const char *msg_log_types[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};
};

}  // namespace Log

#endif
