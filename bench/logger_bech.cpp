#include <benchmark/benchmark.h>
#include "logger.h"

class NullSink : public Log::ILogSink<NullSink> {
public:
    void send(const Log::messageType &msgType, const char *data, size_t size) const {
        // do nothing
    }
};

class EmptyProvider {
public:
    size_t getProcessName(char *buffer, size_t bufferSize) const { return 0; }

    size_t getThreadId(char *buffer, size_t bufferSize) const { return 0; }

    size_t getCurrentDate(char *buffer, size_t bufferSize) const { return 0; }

    size_t getCurrentTime(char *buffer, size_t bufferSize) const { return 0; }
};

static void BM_CreateMessage(benchmark::State &state) {
    const EmptyProvider emptyProvider;
    const NullSink nullSink;
    Log::Logger<EmptyProvider, NullSink> my_logger(emptyProvider, nullSink);
    my_logger.setLogLevel(Log::DebugMsg);
    my_logger.setLogPattern("%{type} file %{file} function %{function} line %{line} %{message}");

    char buffer[LOGGER_MAX_STR_SIZE];
    constexpr Log::LogRecord l = {Log::DebugMsg, "main.cpp",     sizeof("main.cpp"),
                                  "main",        sizeof("main"), 18};

    for (auto _ : state) {
        my_logger.createMessage(buffer, sizeof(buffer), l, "test");
    }
}

BENCHMARK(BM_CreateMessage);

static void BM_Stdprint(benchmark::State &state) {
    char buffer[LOGGER_MAX_STR_SIZE];
    for (auto _ : state) {
        std::snprintf(buffer, sizeof(buffer), "%d file %s function %s line %d %s", Log::DebugMsg,
                      "main.cpp", "main", 18, "test");
    }
}

BENCHMARK(BM_Stdprint);

static void BM_logging(benchmark::State &state) {
    const EmptyProvider emptyProvider;
    const NullSink nullSink;
    Log::Logger<EmptyProvider, NullSink> my_logger(emptyProvider, nullSink);
    my_logger.setLogLevel(Log::DebugMsg);
    my_logger.setLogPattern("%{type} file %{file} function %{function} line %{line} %{message}");

    for (auto _ : state) {
        Debug(my_logger, "test");
    }
}

BENCHMARK(BM_logging);

int main(int argc, char *argv[]) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    return 0;
}
