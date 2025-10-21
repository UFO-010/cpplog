#include <benchmark/benchmark.h>
#include "logger.h"

class NullSink : public Log::ILogSink {
public:
    void send(const Log::messageType &msgType, const char *data, size_t size) override {}
};

class EmptyProvider {
public:
    const char *getProcessName(char *buffer, size_t bufferSize) const { return buffer; }

    const char *getThreadId(char *buffer, size_t bufferSize) const { return buffer; }

    const char *getCurrentDate(char *buffer, size_t bufferSize) const { return buffer; }

    const char *getCurrentTime(char *buffer, size_t bufferSize) const { return buffer; }
};

EmptyProvider emptyProvider;
Log::Logger<EmptyProvider> my_logger(emptyProvider);

static void LoggerSetup() {
    my_logger.setLogLevel(Log::DebugMsg);
    static EmptyProvider mockProvider;
    static NullSink nullSink;
    // Log::Logger::setDataProvider(&mockProvider);
    my_logger.addSink(&nullSink);
    my_logger.setLogPattern(
        "%{type} %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");
}

static void BM_CreateMessage(benchmark::State &state) {
    char buffer[LOGGER_MAX_STR_SIZE];
    for (auto _ : state) {
        my_logger.createMessage(Log::DebugMsg, "main.cpp", "main", 18, "test", buffer,
                                sizeof(buffer));
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

int main(int argc, char *argv[]) {
    LoggerSetup();
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
    return 0;
}
