#ifndef DEFAULTPROVIDER_H
#define DEFAULTPROVIDER_H

#include <cstring>
#include <ctime>
#include <fstream>

#if defined(__linux__) || defined(__APPLE__)
    #include <sys/syscall.h>
    #include <unistd.h>
#endif

#if defined(_WIN32)
    #include <processthreadsapi.h>
    #include <psapi.h>

    #define localtime_r(T, Tm) (localtime_s(Tm, T) ? nullptr : Tm)
#endif

class DefaultDataProvider {
public:
    DefaultDataProvider() { getCurrentProcessName(); }

    ~DefaultDataProvider() = default;

    size_t getProcessName(char *buffer, size_t bufferSize) const {
        size_t size = current_process.size();
        if (size >= bufferSize) {
            return 0;
        }
        std::memcpy(buffer, current_process.data(), size);
        return size;
    }

    size_t getThreadId(char *buffer, size_t bufferSize) const {
#if defined(__linux__)
        pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
        int len = std::snprintf(buffer, bufferSize, "%d", tid);
        return static_cast<size_t>(len);
#elif defined(_WIN32)
        DWORD tid = GetCurrentThreadId();
        int len = std::snprintf(buffer, bufferSize, "%lu", static_cast<unsigned long>(tid));
        return len;
#else
        std::strncpy(buffer, "unknown", bufferSize - 1);
        buffer[sizeof("unknown") - 1] = '\0';
        return sizeof("unknown");
#endif
    }

    size_t getCurrentDate(char *buffer, size_t bufferSize) const {
        time_t timestamp = 0;
        time(&timestamp);
        struct tm datetime = {};

        if (localtime_r(&timestamp, &datetime) != nullptr) {
            size_t len = std::strftime(buffer, bufferSize, "%d.%m.%Y", &datetime);
            return len;
        }
        return 0;
    }

    size_t getCurrentTime(char *buffer, size_t bufferSize) const {
        time_t timestamp = 0;
        time(&timestamp);
        struct tm datetime = {};

        if (localtime_r(&timestamp, &datetime) != nullptr) {
            size_t len = std::strftime(buffer, bufferSize, "%H:%M:%S", &datetime);
            return len;
        }
        return 0;
    }

private:
#if defined(__linux__)
    size_t getCurrentProcessName() {
        std::ifstream file("/proc/self/comm");
        if (file.is_open()) {
            std::getline(file, current_process);
            file.close();
            return current_process.size();
        } else {
            current_process.append(std::to_string(getpid()));
            return current_process.size();
        }
    }
#elif defined(_WIN32)
    size_t getCurrentProcess(char *buffer, size_t bufferSize) {
        DWORD pid = GetCurrentProcessId();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProcess != NULL) {
            char path_buffer[MAX_PATH];
            DWORD size = MAX_PATH;

            if (QueryFullProcessImageNameA(hProcess, 0, path_buffer, &size)) {
                char *last_backslash = std::strrchr(path_buffer, '\\');
                const char *name_to_copy = last_backslash ? (last_backslash + 1) : path_buffer;
                std::strncpy(buffer, name_to_copy, bufferSize - 1);
                buffer[bufferSize - 1] = '\0';
                CloseHandle(hProcess);
                return buffer;
            }
            CloseHandle(hProcess);
        }

        int len = std::snprintf(buffer, bufferSize, "%lu", static_cast<unsigned long>(pid));
        return static_cast<size_t>(len);
    }
#else
    const size_t getCurrentProcess(char *buffer, size_t bufferSize) { buffer[0] = '\0'; }
#endif
    static const size_t proc_len = 64;
    std::string current_process;
};

#if defined(_WIN32)
    #undef localtime_r
#endif

#endif
