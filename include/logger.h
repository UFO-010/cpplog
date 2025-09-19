#ifndef LOGGER_H
#define LOGGER_H

#include <string.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
#endif

#if defined(__GNUC__) || defined(__clang__)

    #define Debug(message) Log::Logger::debug(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Info(message) Log::Logger::info(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Warning(message) Log::Logger::warning(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Error(message) Log::Logger::error(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)
    #define Fatal(message) Log::Logger::fatal(__FILE__, __PRETTY_FUNCTION__, __LINE__, message)

    #define sDebug() Log::MsgSender(Log::DebugMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sInfo() Log::MsgSender(Log::InfoMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sWarning() Log::MsgSender(Log::WarningMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sError() Log::MsgSender(Log::ErrorMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sFatal() Log::MsgSender(Log::FatalMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)

#elif _MSC_VER && !__INTEL_COMPILER

    #define Debug(message) Log::Logger::debug(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Info(message) Log::Logger::info(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Warning(message) Log::Logger::warning(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Error(message) Log::Logger::error(__FILE__, __FUNCSIG__, __LINE__, message)
    #define Fatal(message) Logger::fatal(__FILE__, __FUNCSIG__, __LINE__, message)

    #define sDebug() Log::MsgSender(Log::messageType::Debug, __FILE__, __FUNCSIG__, __LINE__)
    #define sInfo() Log::MsgSender(Log::messageType::Info, __FILE__, __FUNCSIG__, __LINE__)
    #define sWarning() Log::MsgSender(Log::messageType::Warning, __FILE__, __FUNCSIG__, __LINE__)
    #define sError() Log::MsgSender(Log::messageType::Error, __FILE__, __FUNCSIG__, __LINE__)
    #define sFatal() Log::MsgSender(Log::messageType::Fatal, __FILE__, __FUNCSIG__, __LINE__)

#elif

    // __func__ supported since c++11
    #define Debug(message) Log::Logger::debug(__FILE__, __func__, __LINE__, message)
    #define Info(message) Log::Logger::info(__FILE__, __func__, __LINE__, message)
    #define Warning(message) Log::Logger::warning(__FILE__, __func__, __LINE__, message)
    #define Error(message) Log::Logger::error(__FILE__, __func__, __LINE__, message)
    #define Fatal(message) Log::Logger::fatal(__FILE__, __func__, __LINE__, message)

    #define sDebug() Log::MsgSender(Log::messageType::Debug, __FILE__, __func__, __LINE__)
    #define sInfo() Log::MsgSender(Log::messageType::Info, __FILE__, __func__, __LINE__)
    #define sWarning() Log::MsgSender(Log::messageType::Warning, __FILE__, __func__, __LINE__)
    #define sError() Log::MsgSender(Log::messageType::Error, __FILE__, __func__, __LINE__)
    #define sFatal() Log::MsgSender(Log::messageType::Fatal, __FILE__, __func__, __LINE__)

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
    virtual ~ILogSink() = default;
    virtual void send(const messageType &msgType, const char *data, size_t size) = 0;
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
    static void setLogLevel(int level);
    static int getLevel();

    static void setLogFile(const char *file);

    static void setMessagePattern(const std::string &str);

    static void setMessageHandler(void (*handler)(const messageType &msgType,
                                                  const std::string &message));

    static void debug(const std::string &file,
                      const std::string &func,
                      int line,
                      const std::string &str = nullptr);

    static void info(const std::string &file,
                     const std::string &func,
                     int line,
                     const std::string &str = nullptr);

    static void warning(const std::string &file,
                        const std::string &func,
                        int line,
                        const std::string &str = nullptr);

    static void error(const std::string &file,
                      const std::string &func,
                      int line,
                      const std::string &str = nullptr);

    static void fatal(const std::string &file,
                      const std::string &func,
                      int line,
                      const std::string &str = nullptr);

    static std::string createMessage(const messageType &msgType,
                                     const std::string &file,
                                     const std::string &func,
                                     const int line,
                                     const std::string &str = nullptr);

    static void addSink(ILogSink *sink) { sinks.push_back(sink); }

private:
    Logger();
    ~Logger();

    Logger(Logger const &) = delete;
    Logger(const Logger &&) = delete;
    Logger &operator=(Logger const &) = delete;

    static std::string getCurrentProcess();

#if defined(_WIN32)
    static HANDLE getWinConsoleHandle(const std::streambuf *osbuf);
    static bool setWinConsoleAnsiCols(const std::streambuf *osbuf);
#endif
    static int log_level;
    static std::vector<ILogSink *> sinks;

    static std::string current_process;
    /// holds pointers to tokens, so the output will look the  same as @brief setMessagePattern
    static std::vector<const std::string *> tokens_pos;
    /// holds messages that placed after tokens passed in  @brief setMessagePatter
    static std::vector<std::string> tokens_messages;

    static std::ofstream ofs;
    static std::mutex mtx;

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
    /// terminal colors for logging message
    static const char *msg_colors[];
};

inline void Logger::setMessageHandler(void (*_handler)(const messageType &msgType,
                                                       const std::string &message)) {
    user_handler = _handler;
}

/**
 * @brief Logger::setMessagePattern
 * @param str Output message pattern
 *
 * Define ouput logging message format to look like.
 *
 * Options : "%{date}"; "%{time}"; "%{type}"; "%{file}"; "%{thread}";
 * "%{function}"; "%{line}"; "%{pid}"; "%{message}".
 * Always put separators between options
 * Example: "%{date} %{time}"
 * Output: "<current date> <current time>"
 * Example: "%{date}%{time}"
 * Output: "<current date>%{time}"
 */
inline void Logger::setMessagePattern(const std::string &str) {
#if defined(_WIN32)
    if (!setWinConsoleAnsiCols(std::cout.rdbuf())) {
        ansi_cols_support = false;
    }
#endif

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
                current_process = getCurrentProcess();
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
    //     tokens_messages.resize(found_toks);
}

/**
 * @brief The MsgSender class
 *
 * MsgSender is used to generate messages for @brief The Logger class.
 * Constructor will be called every time when logging with operator << is
 * started, destructor will be called at the last call of operator <<. In the
 * destructor all stored elements is sended to @brief Logger.
 *
 * Example:
 * MsgSender(messageType::Debug) << "a" << "\n";
 * |                                          |
 * constructor called     destructor called, passing elements to @brief
 * Logger "a\n" is passed to @brief log_debug(), wrapper of @brief Logger
 *
 * Stack allocations preffered to prevent data races
 *
 * Used through macros sDebug(), sInfo(), sWarning(), sCrirical(), sFatal()
 */
class MsgSender {
public:
    MsgSender(const messageType &msgType)
        : st(),
          message_type(msgType),
          file(),
          function(),
          line() {
        st.str().reserve(128);
    }

    MsgSender(const messageType &msgType, const char *file_name, const char *func, int current_line)
        : st(),
          message_type(msgType),
          file(file_name),
          function(func),
          line(current_line) {
        st.str().reserve(128);
    }

    ~MsgSender() {
        switch (message_type) {
            case DebugMsg:
                Logger::debug(file, function, line, st.str());
                break;
            case InfoMsg:
                Logger::info(file, function, line, st.str());
                break;
            case WarningMsg:
                Logger::warning(file, function, line, st.str());
                break;
            case ErrorMsg:
                Logger::error(file, function, line, st.str());
                break;
            case FatalMsg:
                Logger::fatal(file, function, line, st.str());
                break;
            default:
                break;
        }
    }

    MsgSender &operator<<(char str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(unsigned short str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(signed short str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(unsigned int str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(signed int str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(unsigned long str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(signed long str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(float str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(double str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(const char *str) {
        st << str;
        return *this;
    }

    MsgSender &operator<<(const std::string &str) {
        st << str;
        return *this;
    }

protected:
    std::stringstream st;
    int message_type;
    const char *file;
    const char *function;
    int line;
};

}  // namespace Log

#endif
