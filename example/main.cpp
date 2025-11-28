
#include <thread>

#include "../include/logger.h"
#include "logger_config.h"
#include "../include/console_sink.h"
#include "../include/desktop_provider.h"

using MyConfig = Log::Config::Traits<Log::Config::Default>;

void thread_func1(Log::Logger<MyConfig, PlatformDataProvider<Desktop>, ConsoleSink> const &log) {
    for (int i = 0; i < 1000; i++) {
        Warning(log, "thread 1\n", sizeof("thread 1\n"));  // simple log
        Fatal(log, "thread 1\n", sizeof("thread 1\n"));    // simple log
    }
}

void thread_func2(Log::Logger<MyConfig, PlatformDataProvider<Desktop>, ConsoleSink> const &log) {
    for (int i = 0; i < 1000; i++) {
        Info(log, "thread 2\n", sizeof("thread 2\n"));   // simple log
        Error(log, "thread 2\n", sizeof("thread 2\n"));  // simple log
    }
}

int main() {
    const PlatformDataProvider<Desktop> defaultDataProvider;  // example simple data provider
    const ConsoleSink consoleSink;  // example sink that prints data to console
    Log::Logger myLogger(defaultDataProvider, consoleSink);

    consoleSink.colorize(true);
    myLogger.setLogLevel(Log::level::DebugMsg);  // set minimum logging level

    //  set message pattern
    myLogger.setLogPattern(
        "%{level}\t %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");

    Error(myLogger, "aaa\n", sizeof("aaa\n"));  // simple log

    auto thread1 = std::thread(thread_func1, std::ref(myLogger));
    auto thread2 = std::thread(thread_func2, std::ref(myLogger));
    thread1.join();
    thread2.join();

    return 0;
}
