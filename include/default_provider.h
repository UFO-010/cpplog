#ifndef DEFAULTPROVIDER_H
#define DEFAULTPROVIDER_H

#include "logger.h"
#include <cstring>
#include <ctime>

#if defined(__linux__) || defined(__APPLE__)
    #include <sys/syscall.h>
    #include <unistd.h>
    #include <fstream>
#endif

#if defined(_WIN32)
    #include <processthreadsapi.h>
    #include <psapi.h>

    #define localtime_r(T, Tm) (localtime_s(Tm, T) ? nullptr : Tm)
#endif

class DefaultDataProvider : public Log::DataProvider {
public:
    DefaultDataProvider() { getCurrentProcessName(current_process, proc_len); }

    const char *getProcessName(char *buffer, size_t bufferSize) const override {
        size_t size = proc_len > bufferSize ? bufferSize : proc_len;
        std::memcpy(buffer, current_process, size);
        return buffer;
    }

    const char *getThreadId(char *buffer, size_t bufferSize) const override {
#if defined(__linux__)
        pid_t tid = static_cast<pid_t>(syscall(SYS_gettid));
        std::snprintf(buffer, bufferSize, "%d", tid);
        return buffer;
#elif defined(_WIN32)
        DWORD tid = GetCurrentThreadId();
        std::snprintf(buffer, bufferSize, "%lu", static_cast<unsigned long>(tid));
        return buffer;
#else
        std::strncpy(buffer, "unknown", bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        return buffer;
#endif
    }

    const char *getCurrentDate(char *buffer, size_t bufferSize) const override {
        time_t timestamp = 0;
        time(&timestamp);
        struct tm datetime = {};

        if (localtime_r(&timestamp, &datetime) != nullptr) {
            std::strftime(buffer, bufferSize, "%d.%m.%Y", &datetime);
            return buffer;
        }
        return nullptr;
    }

    const char *getCurrentTime(char *buffer, size_t bufferSize) const override {
        time_t timestamp = 0;
        time(&timestamp);
        struct tm datetime = {};

        if (localtime_r(&timestamp, &datetime) != nullptr) {
            std::strftime(buffer, bufferSize, "%H:%M:%S", &datetime);
            return buffer;
        }
        return nullptr;
    }

private:
#if defined(__linux__)
    const char *getCurrentProcessName(char *buffer, size_t bufferSize) {
        std::ifstream file("/proc/self/comm");
        if (file) {
            file.getline(buffer, bufferSize);
            size_t len = std::strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            return buffer;
        } else {
            std::snprintf(buffer, bufferSize, "%d", static_cast<int>(getpid()));
            return buffer;
        }
    }
#elif defined(_WIN32)
    const char *getCurrentProcess(char *buffer, size_t bufferSize) {
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

        std::snprintf(buffer, bufferSize, "%lu", static_cast<unsigned long>(pid));
        return buffer;
    }
#else
    const char *getCurrentProcess(char *buffer, size_t bufferSize) { buffer[0] = '\0'; }
#endif
    static const size_t proc_len = 64;
    char current_process[proc_len];
};

#if defined(_WIN32)
    #undef localtime_r
#endif

#endif
