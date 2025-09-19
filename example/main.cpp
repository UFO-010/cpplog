
#include <stdio.h>

#include <thread>

#include "logger.h"
#include "console_sync.h"

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
    Log::Logger::setLogLevel(4);
    Log::Logger::setLogFile("log_file.txt");  // set log file
    auto console = ConsoleSync();
    console.colorize(true);
    Log::Logger::addSink(&console);
    //  set message pattern
    Log::Logger::setMessagePattern(
        "%{type}\t %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");
    Error("aaa\n");  // simple log

    std::thread thread1 = std::thread(&thread_func1);
    std::thread thread2 = std::thread(&thread_func2);
    thread1.join();
    thread2.join();
}
