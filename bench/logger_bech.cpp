#include <benchmark/benchmark.h>
#include "logger.h"

class NullSink : public Log::ILogSink<NullSink> {
public:
    void send(const Log::messageType &msgType, const char *data, size_t size) const {}
};

class EmptyProvider {
public:
    size_t getProcessName(char *buffer, size_t bufferSize) const { return 0; }

    size_t getThreadId(char *buffer, size_t bufferSize) const { return 0; }

    size_t getCurrentDate(char *buffer, size_t bufferSize) const { return 0; }

    size_t getCurrentTime(char *buffer, size_t bufferSize) const { return 0; }
};

const EmptyProvider emptyProvider;
const NullSink nullSink;
Log::Logger<EmptyProvider, NullSink> my_logger(emptyProvider, nullSink);
char buffer[LOGGER_MAX_STR_SIZE];

static void LoggerSetup() {
    my_logger.setLogLevel(Log::DebugMsg);
    my_logger.setLogPattern(
        "%{type} %{date} %{time} %{pid} file %{file} "
        "function %{function} line %{line} %{message}");
}

static void BM_CreateMessage(benchmark::State &state) {
    for (auto _ : state) {
        my_logger.createMessage(Log::DebugMsg, "main.cpp", sizeof("main.cpp"), "main",
                                sizeof("main"), 18, "test", buffer, sizeof(buffer));
    }
}

BENCHMARK(BM_CreateMessage);

static void BM_Stdprint(benchmark::State &state) {
    for (auto _ : state) {
        std::snprintf(buffer, sizeof(buffer), "%d file %s function %s line %d %s", Log::DebugMsg,
                      "main.cpp", "main", 18, "test");
    }
}

BENCHMARK(BM_Stdprint);

static void BM_logging(benchmark::State &state) {
    for (auto _ : state) {
        Debug(my_logger, "");
    }
}

BENCHMARK(BM_logging);

int main(int argc, char *argv[]) {
    LoggerSetup();
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
    return 0;
}
