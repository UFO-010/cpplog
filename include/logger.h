#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>

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
    inline static void setLogLevel(int level) { log_level = level; }

    inline static int getLevel() { return log_level; }

    static void setMessagePattern(const std::string &str);

    static void setUserHandler(void (*_handler)(const messageType &msgType,
                                                const std::string &message)) {
        user_handler = _handler;
    }

    static void debug(const std::string &file,
                      const std::string &func,
                      int line,
                      const std::string &str = nullptr) {
        if (log_level < DebugMsg) {
            return;
        }

        std::string msg = createMessage(DebugMsg, file, func, line, str);

        for (auto sink : sinks) {
            sink->send(DebugMsg, msg.c_str(), msg.size());
        }

        if (user_handler != nullptr) {
            user_handler(DebugMsg, msg);
        }
    }

    static void info(const std::string &file,
                     const std::string &func,
                     int line,
                     const std::string &str = nullptr) {
        if (log_level < InfoMsg) {
            return;
        }

        std::string msg = createMessage(InfoMsg, file, func, line, str);

        for (auto sink : sinks) {
            sink->send(InfoMsg, msg.c_str(), msg.size());
        }
        if (user_handler != nullptr) {
            user_handler(InfoMsg, msg);
        }
    }

    static void warning(const std::string &file,
                        const std::string &func,
                        int line,
                        const std::string &str = nullptr) {
        if (log_level < WarningMsg) {
            return;
        }

        std::string msg = createMessage(WarningMsg, file, func, line, str);

        for (auto sink : sinks) {
            sink->send(WarningMsg, msg.c_str(), msg.size());
        }
        if (user_handler != nullptr) {
            user_handler(WarningMsg, msg);
        }
    }

    static void error(const std::string &file,
                      const std::string &func,
                      int line,
                      const std::string &str = nullptr) {
        if (log_level < ErrorMsg) {
            return;
        }

        std::string msg = createMessage(ErrorMsg, file, func, line, str);

        for (auto sink : sinks) {
            sink->send(ErrorMsg, msg.c_str(), msg.size());
        }
        if (user_handler != nullptr) {
            user_handler(ErrorMsg, msg);
        }
    }

    static void fatal(const std::string &file,
                      const std::string &func,
                      int line,
                      const std::string &str = nullptr) {
        if (log_level < FatalMsg) {
            return;
        }

        std::string msg = createMessage(FatalMsg, file, func, line, str);
        for (auto sink : sinks) {
            sink->send(FatalMsg, msg.c_str(), msg.size());
        }

        if (user_handler != nullptr) {
            user_handler(FatalMsg, msg);
        }
    }

    /**
     * @brief Logger::createMessage
     * @param msgType type of message, see @param messageType
     * @param file name of file function called from
     * @param func name of function this function called from
     * @param line number of function this function called from
     * @param str input string to print in log message
     * @param msg poitner to output log message
     *      * Creates log message with specified in @brief Logger::setMessagePattern
     * view and print it to terminal. To prevent errors we write terminal
     * colors directly to the message
     */
    static std::string createMessage(const messageType &msgType,
                                     const std::string &file,
                                     const std::string &func,
                                     const int line,
                                     const std::string &str = nullptr) {
        std::string msg;
        msg.reserve(512);

        for (size_t i = 0; i < tokens_pos.size(); i++) {
            if (tokens_pos[i] == &tok_date) {
                msg.append(tokens_messages[i]);
                msg.append(data_provider->getCurrentDate(msg.data() + msg.size(), 16));
            }
            if (tokens_pos[i] == &tok_time) {
                msg.append(tokens_messages[i]);
                msg.append(data_provider->getCurrentTime(msg.data() + msg.size(), 16));
            }
            if (tokens_pos[i] == &tok_type) {
                msg.append(tokens_messages[i]);
                msg.append(msg_log_types[msgType]);
            }
            if (tokens_pos[i] == &tok_file) {
                msg.append(tokens_messages[i]);
                msg.append(file);
            }
            if (tokens_pos[i] == &tok_thread) {
                msg.append(tokens_messages[i]);
                msg.append(data_provider->getThreadId(msg.data() + msg.size(), 16));
            }
            if (tokens_pos[i] == &tok_func) {
                msg.append(tokens_messages[i]);
                msg.append(func);
            }
            if (tokens_pos[i] == &tok_line) {
                msg.append(tokens_messages[i]);
                msg.append(std::to_string(line));
            }
            if (tokens_pos[i] == &tok_pid) {
                msg.append(tokens_messages[i]);
                msg.append(data_provider->getProcessName(msg.data() + msg.size(), 64));
            }
            if (tokens_pos[i] == &tok_message) {
                msg.append(tokens_messages[i]);
                msg.append(str);
            }
        }
        return msg;
    }

    static void addSink(ILogSink *sink) { sinks.push_back(sink); }

    static void setDataProvider(const DataProvider *provider) { data_provider = provider; }

private:
    Logger() {}

    static int log_level;
    static std::vector<ILogSink *> sinks;
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

/**
 * @brief Logger::setMessagePattern
 * @param str Output message pattern
 *
 * Define ouput logging message format to look like.
 *
 * Options : "%{date}"; "%{time}"; "%{type}"; "%{file}"; "%{thread}";
 * "%{function}"; "%{line}"; "%{pid}"; "%{message}".
 * Always put separators between options
 * @example "%{date} %{time}"
 * Output: "<current date> <current time>"
 * @example "%{date}%{time}"
 * Output: "<current date>%{time}"
 */
inline void Logger::setMessagePattern(const std::string &str) {
    tokens_messages.clear();
    tokens_pos.clear();
    tokens_messages.resize(9);

    bool maybe_tok_start = false;
    int found_toks = 0;
    std::string maybe_token = "";

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '%') {
            maybe_token = "";
            maybe_token.reserve(64);
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

}  // namespace Log

#endif
