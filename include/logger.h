#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <cstring>

#include "logger_config.h"

#if defined(__GNUC__) || defined(__clang__)

    #define Debug(message) Log::Logger::debug(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Info(message) Log::Logger::info(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Warning(message) Log::Logger::warning(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Error(message) Log::Logger::error(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Fatal(message) Log::Logger::fatal(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)

#elif _MSC_VER && !__INTEL_COMPILER

    #define Debug(message) Log::Logger::debug(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Info(message) Log::Logger::info(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Warning(message) Log::Logger::warning(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Error(message) Log::Logger::error(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Fatal(message) Logger::fatal(__FILE__, __FUNCSIG__, __LINE__, message)

#elif

    #define Debug(message) Log::Logger::debug(__FILE__, __func__, __LINE__, message)
    #define Info(message) Log::Logger::info(__FILE__, __func__, __LINE__, message)
    #define Warning(message) Log::Logger::warning(__FILE__, __func__, __LINE__, message)
    #define Error(message) Log::Logger::error(__FILE__, __func__, __LINE__, message)
    #define Fatal(message) Log::Logger::fatal(__FILE__, __func__, __LINE__, message)

#endif

namespace Log {

enum messageType : int {
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
     * @brief Logger::setMessagePattern
     * @param str Output message pattern
     *      * Define ouput logging message format to look like.
     *      * Options : "%{date}"; "%{time}"; "%{type}"; "%{file}"; "%{thread}";
     * "%{function}"; "%{line}"; "%{pid}"; "%{message}".
     * Always put separators between options
     * @example "%{date} %{time}"
     * Output: "<current date> <current time>"
     * @example "%{date}%{time}"
     * Output: "<current date>%{time}"
     */
    static void setMessagePattern(const std::string &str) {
        tokens_messages.clear();
        tokens_pos.clear();
        tokens_messages.resize(LOGGER_MAX_MESSAGES);

        bool maybe_tok_start = false;
        int found_toks = 0;
        std::string maybe_token;

        for (size_t i = 0; i < str.size(); i++) {
            if (str[i] == '%') {
                maybe_token = "";
                maybe_token.reserve(LOGGER_MAX_TOK_SIZE);
                maybe_token.push_back(str[i]);
                i++;
                if ((i < str.size()) && (str[i] == '{')) {
                    maybe_tok_start = true;
                    maybe_token.push_back(str[i]);
                    i++;
                }
            }

            if (maybe_tok_start) {
                maybe_token.push_back(str[i]);
            }

            if ((str[i] == '}') && maybe_tok_start) {
                if (maybe_token == tok_date) {
                    tokens_pos.push_back(&tok_date);
                    ++found_toks;
                } else if (maybe_token == tok_time) {
                    tokens_pos.push_back(&tok_time);
                    ++found_toks;
                } else if (maybe_token == tok_type) {
                    tokens_pos.push_back(&tok_type);
                    ++found_toks;
                } else if (maybe_token == tok_file) {
                    tokens_pos.push_back(&tok_file);
                    ++found_toks;
                } else if (maybe_token == tok_thread) {
                    tokens_pos.push_back(&tok_thread);
                    ++found_toks;
                } else if (maybe_token == tok_func) {
                    tokens_pos.push_back(&tok_func);
                    ++found_toks;
                } else if (maybe_token == tok_line) {
                    tokens_pos.push_back(&tok_line);
                    ++found_toks;
                } else if (maybe_token == tok_pid) {
                    tokens_pos.push_back(&tok_pid);
                    ++found_toks;
                } else if (maybe_token == tok_message) {
                    tokens_pos.push_back(&tok_message);
                    ++found_toks;
                }
                maybe_tok_start = false;
                i++;
                maybe_token = "";
            }

            if (!maybe_tok_start && i < str.size()) {
                tokens_messages[found_toks].push_back(str[i]);
            }
        }
    }

    static void setUserHandler(void (*_handler)(const messageType &msgType,
                                                const std::string &message)) {
        user_handler = _handler;
    }

    static void debug(const char *file, const char *func, int line, const char *str = nullptr) {
        if (log_level < DebugMsg) {
            return;
        }

        static char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(DebugMsg, file, func, line, str, msg, sizeof(msg));
#if ENABLE_SINKS == 1
        for (int i = 0; i < sink_count; i++) {
            sinks[i]->send(DebugMsg, msg, msg_size);
        }
#endif
#if ENABLE_PRINT_CALLBACK == 1
        if (user_handler != nullptr) {
            user_handler(DebugMsg, msg);
        }
#endif
    }

    static void info(const char *file, const char *func, int line, const char *str = nullptr) {
        if (log_level < InfoMsg) {
            return;
        }

        static char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(InfoMsg, file, func, line, str, msg, sizeof(msg));
#if ENABLE_SINKS == 1
        for (int i = 0; i < sink_count; i++) {
            sinks[i]->send(InfoMsg, msg, msg_size);
        }
#endif
#if ENABLE_PRINT_CALLBACK == 1
        if (user_handler != nullptr) {
            user_handler(InfoMsg, msg);
        }
#endif
    }

    static void warning(const char *file, const char *func, int line, const char *str = nullptr) {
        if (log_level < WarningMsg) {
            return;
        }

        static char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(WarningMsg, file, func, line, str, msg, sizeof(msg));
#if ENABLE_SINKS == 1
        for (int i = 0; i < sink_count; i++) {
            sinks[i]->send(WarningMsg, msg, msg_size);
        }
#endif
#if ENABLE_PRINT_CALLBACK == 1
        if (user_handler != nullptr) {
            user_handler(WarningMsg, msg);
        }
#endif
    }

    static void error(const char *file, const char *func, int line, const char *str = nullptr) {
        if (log_level < ErrorMsg) {
            return;
        }

        static char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(ErrorMsg, file, func, line, str, msg, sizeof(msg));
#if ENABLE_SINKS == 1
        for (int i = 0; i < sink_count; i++) {
            sinks[i]->send(ErrorMsg, msg, msg_size);
        }
#endif
#if ENABLE_PRINT_CALLBACK == 1
        if (user_handler != nullptr) {
            user_handler(ErrorMsg, msg);
        }
#endif
    }

    static void fatal(const char *file, const char *func, int line, const char *str = nullptr) {
        if (log_level < FatalMsg) {
            return;
        }

        static char msg[LOGGER_MAX_STR_SIZE];
        size_t msg_size = createMessage(FatalMsg, file, func, line, str, msg, sizeof(msg));
#if ENABLE_SINKS == 1
        for (int i = 0; i < sink_count; i++) {
            sinks[i]->send(FatalMsg, msg, msg_size);
        }
#endif
#if ENABLE_PRINT_CALLBACK == 1
        if (user_handler != nullptr) {
            user_handler(FatalMsg, msg);
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
        for (size_t i = 0; i < tokens_pos.size(); i++) {
            if (tokens_pos[i] == &tok_date) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                data_provider->getCurrentDate(temp, sizeof(temp));
                appendC(temp);
            }
            if (tokens_pos[i] == &tok_time) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                data_provider->getCurrentTime(temp, sizeof(temp));
                appendC(temp);
            }
            if (tokens_pos[i] == &tok_type) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                appendC(msg_log_types[msgType]);
            }
            if (tokens_pos[i] == &tok_file) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                appendC(file);
            }
            if (tokens_pos[i] == &tok_thread) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                data_provider->getThreadId(temp, sizeof(temp));
                appendC(temp);
            }
            if (tokens_pos[i] == &tok_func) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                appendC(func);
            }
            if (tokens_pos[i] == &tok_line) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                appendInt(line);
            }
            if (tokens_pos[i] == &tok_pid) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                data_provider->getProcessName(temp, sizeof(temp));
                appendC(temp);
            }
            if (tokens_pos[i] == &tok_message) {
                append(tokens_messages[i].data(), tokens_messages[i].size());
                appendC(str);
            }
        }
        return pos;
    }

    static void addSink(ILogSink *sink) {
        if (sink_count < LOGGER_MAX_SINKS) {
            sinks[sink_count] = sink;
            sink_count++;
        }
    }

    static void setDataProvider(const DataProvider *provider) { data_provider = provider; }

private:
    Logger() {}

    static int log_level;
    static ILogSink *sinks[LOGGER_MAX_SINKS];
    static int sink_count;
    static const DataProvider *data_provider;

    /// holds pointers to tokens, so the output will look the  same as @brief setMessagePattern
    static std::vector<const std::string *> tokens_pos;
    /// holds messages that placed after tokens passed in  @brief setMessagePatter
    static std::vector<std::string> tokens_messages;

    static void (*user_handler)(const messageType &msgType, const std::string &message);

    /// token for message pattern
    static const std::string tok_date;
    static const std::string tok_time;
    static const std::string tok_type;
    static const std::string tok_file;
    static const std::string tok_thread;
    static const std::string tok_func;
    static const std::string tok_line;
    static const std::string tok_pid;
    static const std::string tok_message;

    /// types of logging level, added to output message
    static const char *msg_log_types[];
};

}  // namespace Log

#endif
