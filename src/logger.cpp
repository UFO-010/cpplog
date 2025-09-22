
#include "logger.h"

#include <ctime>
#include <fstream>

#if defined __linux__
    #include <sys/syscall.h>
    #include <unistd.h>
#endif

#if defined WIN32
    #include <processthreadsapi.h>
    #include <psapi.h>
    #define localtime_r(T, Tm) (localtime_s(Tm, T) ? NULL : Tm)
#endif

namespace Log {
int Logger::log_level = 4;

std::vector<ILogSink *> Logger::sinks = {};

const std::string Logger::tok_date = "%{date}";
const std::string Logger::tok_time = "%{time}";
const std::string Logger::tok_type = "%{type}";
const std::string Logger::tok_file = "%{file}";
const std::string Logger::tok_thread = "%{thread}";
const std::string Logger::tok_func = "%{function}";
const std::string Logger::tok_line = "%{line}";
const std::string Logger::tok_pid = "%{pid}";
const std::string Logger::tok_message = "%{message}";

const char *Logger::msg_log_types[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

std::vector<const std::string *> Logger::tokens_pos = {&tok_type, &tok_message};
std::vector<std::string> Logger::tokens_messages = {"\t", ""};
std::string Logger::current_process = "";

void (*Logger::user_handler)(const messageType &msgType, const std::string &message) = 0;

Logger::Logger() {}

Logger::~Logger() {}

/**
 * @brief Logger::setLogLevel
 * @param level priority of messages to display
 *
 * Set logging level. Messages with a lower priority level will be ignored,
 * see
 * @param messageType
 */
void Logger::setLogLevel(int level) {
    log_level = level;
}

void Logger::debug(const std::string &file,
                   const std::string &func,
                   int line,
                   const std::string &str) {
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

void Logger::info(const std::string &file,
                  const std::string &func,
                  int line,
                  const std::string &str) {
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

void Logger::warning(const std::string &file,
                     const std::string &func,
                     int line,
                     const std::string &str) {
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

void Logger::error(const std::string &file,
                   const std::string &func,
                   int line,
                   const std::string &str) {
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

void Logger::fatal(const std::string &file,
                   const std::string &func,
                   int line,
                   const std::string &str) {
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
 *
 * Creates log message with specified in @brief Logger::setMessagePattern
 * view and print it to terminal. To prevent errors we write terminal
 * colors directly to the message
 */
inline std::string Logger::createMessage(const messageType &msgType,
                                         const std::string &file,
                                         const std::string &func,
                                         const int line,
                                         const std::string &str) {
    std::string msg;
    msg.reserve(512);

    for (size_t i = 0; i < tokens_pos.size(); i++) {
        if (tokens_pos[i] == &tok_date) {
            msg.append(tokens_messages[i]);

            time_t timestamp;
            time(&timestamp);
            struct tm datetime;
            datetime = *localtime_r(&timestamp, &datetime);
            char out[16];  // unsafe
            strftime(out, 16, "%d.%m.%Y", &datetime);
            msg.append(out);
        }
        if (tokens_pos[i] == &tok_time) {
            msg.append(tokens_messages[i]);

            time_t timestamp;
            time(&timestamp);
            struct tm datetime;
            datetime = *localtime_r(&timestamp, &datetime);
            char out[16];  // unsafe
            strftime(out, 16, "%H:%M:%S", &datetime);
            msg.append(out);
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
#if defined(__linux__)
            msg.append(std::to_string(gettid()));
#elif defined(_WIN32)
            msg.append(std::to_string(GetCurrentThreadId()));
#endif
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
            msg.append(current_process);
        }
        if (tokens_pos[i] == &tok_message) {
            msg.append(tokens_messages[i]);
            msg.append(str);
        }
    }
    return msg;
}

int Logger::getLevel() {
    return log_level;
}

#if defined(__linux__)
std::string Logger::getCurrentProcess() {
    std::string pid_str(std::to_string(getpid()));
    std::string path("/proc/");
    path.append(pid_str);
    path.append("/comm");
    std::ifstream file(path);
    if (!file) {
        return pid_str;
    }
    std::string pid_name;
    std::getline(file, pid_name);
    return pid_name;
}
#endif

#if defined(_WIN32)
std::string Logger::getCurrentProcess() {
    uint32_t pid = GetCurrentProcessId();
    std::string result;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess != NULL) {
        char buffer[MAX_PATH];
        GetProcessImageFileNameA(hProcess, buffer, MAX_PATH);
        result = std::string(buffer);
        CloseHandle(hProcess);

        size_t last = 0;
        size_t next = 0;
        char c = 92;  // '\'
        std::string delimiter(1, c);

        while ((next = result.find(delimiter, last)) != std::string::npos) {
            last = next + 1;
        }
        std::string name = result.substr(last);

        return name;
    }

    std::string msg(std::to_string(pid));
    return msg;
}

#endif

}  // namespace Log
