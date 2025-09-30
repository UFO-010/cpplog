#ifndef STREAMLOGGER_H
#define STREAMLOGGER_H

#include <sstream>
#include "logger.h"

#if defined(__GNUC__) || defined(__clang__)
    #define sDebug() \
        Log::MsgSender(Log::messageType::DebugMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sInfo() \
        Log::MsgSender(Log::messageType::InfoMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sWarning() \
        Log::MsgSender(Log::messageType::WarningMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sError() \
        Log::MsgSender(Log::messageType::ErrorMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)
    #define sFatal() \
        Log::MsgSender(Log::messageType::FatalMsg, __FILE__, __PRETTY_FUNCTION__, __LINE__)

#elif _MSC_VER && !__INTEL_COMPILER
    #define sDebug() Log::MsgSender(Log::messageType::DebugMsg, __FILE__, __FUNCSIG__, __LINE__)
    #define sInfo() Log::MsgSender(Log::messageType::InfoMsg, __FILE__, __FUNCSIG__, __LINE__)
    #define sWarning() Log::MsgSender(Log::messageType::WarningMsg, __FILE__, __FUNCSIG__, __LINE__)
    #define sError() Log::MsgSender(Log::messageType::ErrorMsg, __FILE__, __FUNCSIG__, __LINE__)
    #define sFatal() Log::MsgSender(Log::messageType::FatalMsg, __FILE__, __FUNCSIG__, __LINE__)

#elif
    #define sDebug() Log::MsgSender(Log::messageType::DebugMsg, __FILE__, __func__, __LINE__)
    #define sInfo() Log::MsgSender(Log::messageType::InfoMsg, __FILE__, __func__, __LINE__)
    #define sWarning() Log::MsgSender(Log::messageType::WarningMsg, __FILE__, __func__, __LINE__)
    #define sError() Log::MsgSender(Log::messageType::ErrorMsg, __FILE__, __func__, __LINE__)
    #define sFatal() Log::MsgSender(Log::messageType::FatalMsg, __FILE__, __func__, __LINE__)

#endif

namespace Log {

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
        : message_type(msgType),
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

    ~MsgSender() { Logger::log(message_type, file, function, line, st.str().c_str()); }

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

private:
    std::stringstream st;
    messageType message_type;
    const char *file;
    const char *function;
    int line;
};

};  // namespace Log

#endif
