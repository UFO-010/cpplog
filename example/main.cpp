
#include <thread>

#include "../include/logger.h"
#include "../include/console_sink.h"
#include "../include/stream_logger.h"
#include "../include/default_provider.h"

void thread_func1() {
    for (int i = 0; i < 1000; i++) {
        sInfo() << "thread "
                << "1 "
                << "no "
                << "data "
                << "race "
                << "ha-ha"
                << "\n";        // log using text stream
        Warning("thread 1\n");  // simple log
        Fatal("thread 1\n");
    }
}

void thread_func2() {
    for (int i = 0; i < 1000; i++) {
        sDebug() << "thread "
                 << "2 "
                 << "no "
                 << "data "
                 << "race "
                 << "ha-ha"
                 << "\n";     // log using text stream
        Error("thread 2\n");  // simple log
    }
}

int main() {
    Log::Logger::setLogLevel(Log::DebugMsg);
    DefaultDataProvider defaultDataProvider;
    Log::Logger::setDataProvider(&defaultDataProvider);

    auto console = ConsoleSink();
    console.colorize(true);
    Log::Logger::addSink(&console);

    //  set message pattern
    Log::Logger::setLogPattern(
        "%{type}\t %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");

    Error("aaa\n");  // simple log

    std::thread thread1 = std::thread(&thread_func1);
    std::thread thread2 = std::thread(&thread_func2);
    thread1.join();
    thread2.join();
}
