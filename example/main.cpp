
#include <thread>

#include "../include/logger.h"
#include "../include/console_sink.h"
#include "../include/default_provider.h"

DefaultDataProvider defaultDataProvider;  // example simple data provider
ConsoleSink consoleSink;                  // example sink that prints data to console
Log::Logger<DefaultDataProvider, ConsoleSink> myLogger(defaultDataProvider, consoleSink);

void thread_func1() {
    for (int i = 0; i < 1000; i++) {
        Warning(myLogger, "thread 1\n");  // simple log
        Fatal(myLogger, "thread 1\n");
    }
}

void thread_func2() {
    for (int i = 0; i < 1000; i++) {
        Error(myLogger, "thread 2\n");  // simple log
    }
}

int main() {
    consoleSink.colorize(true);
    myLogger.setLogLevel(Log::DebugMsg);

    //  set message pattern
    myLogger.setLogPattern(
        "%{type}\t %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");

    Error(myLogger, "aaa\n");  // simple log

    std::thread thread1 = std::thread(&thread_func1);
    std::thread thread2 = std::thread(&thread_func2);
    thread1.join();
    thread2.join();
}
