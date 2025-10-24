
#include <thread>

#include "../include/logger.h"
#include "../include/console_sink.h"
#include "../include/default_provider.h"

void thread_func1(Log::Logger<DefaultDataProvider, ConsoleSink> log) {
    for (int i = 0; i < 1000; i++) {
        Warning(log, "thread 1\n");  // simple log
        Fatal(log, "thread 1\n");
    }
}

void thread_func2(Log::Logger<DefaultDataProvider, ConsoleSink> log) {
    for (int i = 0; i < 1000; i++) {
        Error(log, "thread 2\n");  // simple log
    }
}

int main() {
    const DefaultDataProvider defaultDataProvider;  // example simple data provider
    const ConsoleSink consoleSink;                  // example sink that prints data to console
    Log::Logger<DefaultDataProvider, ConsoleSink> myLogger(defaultDataProvider, consoleSink);

    consoleSink.colorize(true);
    myLogger.setLogLevel(Log::DebugMsg);

    //  set message pattern
    myLogger.setLogPattern(
        "%{type}\t %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");

    Error(myLogger, "aaa\n");  // simple log

    std::thread thread1 = std::thread(thread_func1, myLogger);
    std::thread thread2 = std::thread(thread_func2, myLogger);
    thread1.join();
    thread2.join();

    return 0;
}
