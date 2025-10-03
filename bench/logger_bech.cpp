#include <benchmark/benchmark.h>
#include "logger.h"

class NullSink : public Log::ILogSink {
public:
    void send(const Log::messageType &msgType, const char *data, size_t size) override {}
};

class EmptyProvider : public Log::DataProvider {
    const char *getProcessName(char *buffer, size_t bufferSize) const override { return buffer; }

    const char *getThreadId(char *buffer, size_t bufferSize) const override { return buffer; }

    const char *getCurrentDate(char *buffer, size_t bufferSize) const override { return buffer; }

    const char *getCurrentTime(char *buffer, size_t bufferSize) const override { return buffer; }
};

static void LoggerSetup() {
    Log::Logger::setLogLevel(Log::DebugMsg);
    static EmptyProvider mockProvider;
    static NullSink nullSink;
    Log::Logger::setDataProvider(&mockProvider);
    Log::Logger::addSink(&nullSink);
    Log::Logger::setLogPattern(
        "%{type} %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");
}

static void BM_CreateMessage(benchmark::State &state) {
    LoggerSetup();
    char buffer[LOGGER_MAX_STR_SIZE];
    for (auto _ : state) {
        Log::Logger::createMessage(Log::DebugMsg, "main.cpp", "main", 18, "test", buffer,
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
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
    return 0;
}
