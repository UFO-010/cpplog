
#include "logger.h"

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

std::string Logger::getCurrentThread() {
#if defined(__linux__)
    return std::to_string(gettid());
#elif defined(_WIN32)
    return std::to_string(GetCurrentThreadId());
#endif
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
